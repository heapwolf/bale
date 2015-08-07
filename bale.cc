#include <iostream>
#include <string>
#include <regex>
#include <vector>
#include <deque>
#include <map>

#include "fs.h"
#include "debug.h"
#include "json11.hpp"
#include "path.h"
#include "docopt.h"

using namespace std;
using namespace nodeuv;
using namespace json11;

Filesystem fs;
Path p;
Debug log("bale", Debug::verbose, std::cerr);

typedef function<void(Error)> Callback;
typedef vector<const string> Defines;
typedef map<const string, pair<const string, const string> > Imports;

vector<const string> include_cache;
deque<const string> include_dirs;
map<const string, pair<const string, const string> > import_cache;
int depth = 0;

regex importRE("import\\s+(\\w+)\\s+\"(.*)\";");
regex exportStartRE("export\\s*(\\w+)\\s*\\{");
regex exportEndRE("\\}(\\s*)$");


string wrap(string str, string name) {
  str = "#ifndef __" + name + "_H\n#define __" + name + "_H\n" + str;
  str = regex_replace(str, exportStartRE, "class " + name + " {");
  str = regex_replace(str, exportEndRE, "};\n#endif\n\n");
  return str;
}


Imports find_imports (string data) {

  Imports imports;

  sregex_token_iterator
    i(data.begin(), data.end(), importRE), iend;

  while(i != iend) {
    smatch sm;
    string s = *i++;
    regex_match(s, sm, importRE);

    if (sm.size()) {
      imports.insert({ sm[0], make_pair(sm[1], sm[2]) });
    }
  }
  return imports;
}


void resolveImport(string basePath, string name, Callback<Error, const string> cb) {

  bool isNamePath = name[0] == '/' || name[0] == '.';
  Error pathReference;

  if (isNamePath) {

    name = p.resolve(fs.cwd(), basePath, name);

    cb(pathReference, name);
    return;
  }

  auto die = [&]() {
    Error notFound;
    notFound.message = "Module not found: " + name;
    notFound.code = 1;
    cb(notFound, name); 
  };

  function<void(string)> visit;
  visit = [&](string pathPart) {

    auto next = p.join(pathPart, "/cc_modules/", name);

    fs.stat(next, [&](auto err, auto stats) {
      if (err) {

        string parent = pathPart.substr(0, pathPart.find_last_of("/\\"));
        if (parent.length() == 0) {
          die();
          return;
        }

        visit(parent); 
      }

      string newPath = p.join(next, "/index.cc");
      fs.stat(newPath, [&](auto err, auto stats) {
        if (err) {
          die();
          return;
        }
        cb(err, newPath);
      });
    });
  };

  visit(basePath);
}


void createClassIds(string basePath, string value, Callback<Error> cb) {

  resolveImport(basePath, value, [&](auto err, auto path) {
    if (err) {
      cb(err);
      return;
    }

    fs.readFile(path, [&](auto err, auto data) {
      if (err) {
        cb(err);
        return;
      }

      depth++;

      auto imports = find_imports(data.toString());
      int counter = imports.size();

      if (import_cache.find(value) == import_cache.end()) {

        string id = "MODULE" + to_string(depth);
        import_cache.insert({ value, make_pair(path, id) });
      }

      if (counter == 0) {
        cb(err);
        return;
      }

      for (auto& import : imports) {

        auto path = get<1>(import.second).c_str();
        createClassIds(basePath, path, [&](auto err) {
          counter = counter - 1;
          if (counter == 0) {
            cb(err);
          }
        });
      }

    });
  });
}


void ensurePath(string path) {

  istringstream ss(path.substr(0, path.find_last_of("/\\")));
  string token;
  string tmp;

  while (std::getline(ss, token, '/')) {
    tmp += token + "/";
    try {
      fs.statSync(tmp.c_str());
    }
    catch(...) {
      fs.mkdirSync(tmp.c_str());
    }
  }
}


void rewriteFiles(string basePath, string path, Callback<Error> cb) {

  int cache_size = import_cache.size();
  int index = 0;
  Error error;

  for (auto& import : import_cache) {

    auto filepath = get<0>(import.second);
    auto id = get<1>(import.second);

    fs.readFile(filepath, [&](auto err, auto data) {

      if (error && ++index == cache_size) {
        cb(error);
        return;
      }

      if (err) {
        error = err;
        return;
      }

      auto imports = find_imports(data.toString());
      string output = data.toString();

      if (id != "MODULE1") {
        output = wrap(output, get<1>(import.second));
      }

      for (auto& i : imports) {
        // the identity of the new class 
        auto importname = get<1>(import_cache[get<1>(i.second)]);

        // the actual path to the file
        auto importpath = get<0>(import_cache[get<1>(i.second)]);

        string statement = importname + " " + get<0>(i.second) + ";";
        output = regex_replace(output, regex(i.first), statement);
      }

      string filename = filepath;

      filename = path + regex_replace(filename, regex(basePath), "");

      if (id != "MODULE1") {
        filename += ".h";
        include_dirs.push_front(filename);
      }

      //log << log.info << "Writing " << filename << endl;

      ensurePath(filename);

      fs.writeFile(filename, output, [&](auto err) {

        if (err) {
          error = err;
          return;
        }

        if (++index == cache_size) {
          cb(error);
        }
      });
    });
  }
}

void build(string path) {

  if (!system(NULL)) {
    log 
      << log.error 
      << "A command processor is not available" 
      << endl;

    exit(EXIT_FAILURE);
  }


  //
  // TODO
  // clean this up...
  //
  try {

    auto packagePath = p.join(path, "/package.json");
    const string data = fs.readFileSync(packagePath).toString();

    string parse_error;
    Json package = Json::parse(data, parse_error);

    string cmd = "";
    auto name = package["name"].string_value();
    auto engines = package["engines"].object_items();
    auto flags = package["flags"].array_items();

    // 
    // TODO
    // Test if the engine exists and that its version number
    // is appropriate for building the current package.
    // For now, just take whatever is in there and use it.
    //
    for (auto &engine : engines) {
      cmd += engine.first + " ";
    }

    //
    // Specify the program entry point
    //
    string builtPath = p.join(path, "./.build");
    string mainFile = package["main"].string_value();
    string indexPath = p.resolve(builtPath, mainFile);
    cmd += indexPath + " ";

    for (auto &flag : flags) {
      cmd += flag.string_value() + " ";
    }

    //
    // Specify the output
    //
    string binPath = p.resolve(path, package["bin"].string_value());
    ensurePath(binPath);
    cmd += "-o " + binPath + " ";

    //
    // Specify the include path
    //
    for (auto &include : include_dirs) {
      cmd += "-include " + include + " ";
    }
    //cout << cmd << endl;
    system(cmd.c_str());

  }
  catch(...) {
    log << log.error << "Could not parse package.json" << endl;
    exit(1);
  }

}

void transpile(string inputFile) {

  auto die = [=](auto err) {
    if (err.message == "ENOENT") {

      log 
        << log.error 
        << "the file could not be read (" + inputFile + ")" 
        << endl;

      exit(1);
    }
   else if (err) {

      log << log.error << err.message << endl;
      exit(1);
    }
  };

  auto input = p.resolve(fs.cwd(), inputFile);
  auto output = p.join(p.dirname(input), "./.build");
  auto basePath = p.resolve(fs.cwd(), p.dirname(inputFile));

  ensurePath(output);

  createClassIds(basePath, input, [&](auto err) {
    if (err) die(err);

    rewriteFiles(basePath, output, [&](auto err) {
      if (err) die(err);

      build(p.dirname(input));
    });
  });
}

void run(string name) {

  if (!system(NULL)) {
    log 
      << log.error 
      << "A command processor is not available" 
      << endl;

    exit(EXIT_FAILURE);
  }

  auto packageName = p.join(fs.cwd(), "/package.json");

  try {
    const string data = fs.readFileSync(packageName).toString();
    string parse_error;
    Json package = Json::parse(data, parse_error);

    auto script = package["scripts"][name].string_value();
    system(script.c_str());
  }
  catch(...) {
    //
    // TODO
    //
  }

}

void install(string name) {

  //
  // TODO
  // http, connect to github, etc.
  //
  cout << "INSTALL" << name << endl;
}

int main(int argc, char* argv[]) {

static const char USAGE[] =
R"(bale

    Usage:
      bale (-i | install) [name]...
      bale (-b | build) <FILE>
      bale (-r | run) <script>...
      bale (-h | --help)
      bale --version

    Arguments:
      [name]        name of the module to install.
      FILE          an input file.
      <script>      a string of bash script.

    Options:
      -h --help     Show this screen.
      --version     Show version.
)";

  map<std::string, docopt::value> 
  args = docopt::docopt(
    USAGE,
    { argv + 1, argv + argc },
    true,
    "Bale 1.0"
  );

  if (args.find("build")->second == (docopt::value) "true") {
    transpile((string) args.find("<FILE>")->second.asString());
  }
  else if (args.find("install")->second == (docopt::value) "true") {
    install((string) args.find("[name]")->second.asString());
  }
  else if (args.find("script")->second == (docopt::value) "true") {
    run((string) args.find("<script>")->second.asString());
  }
}

