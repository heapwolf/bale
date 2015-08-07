#include <iostream>
#include <string>
#include <regex>
#include <vector>
#include <map>
#include "fs.h"
#include "debug.h"
#include "json11.hpp"
#include "path.h"

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
map<const string, pair<const string, const string> > import_cache;
int depth = 0;

regex importRE("import\\s+(\\w+)\\s+\"(.*)\";");
regex exportStartRE("export\\s*(\\w+)\\s*\\{");
regex exportEndRE("\\}(\\s*)$");


string wrap(string str, string name) {
  str = "#ifndef " + name + "\n#define " + name + "\n" + str;
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

  for (auto& import : import_cache) {

    auto filepath = get<0>(import.second);
    auto id = get<1>(import.second);

    fs.readFile(filepath, [&](auto err, auto data) {
      auto imports = find_imports(data.toString());
      string output = data.toString();

      if (id != "MODULE1")
        output = wrap(output, get<1>(import.second));

      for (auto& i : imports) {
        // the identity of the new class 
        auto importname = get<1>(import_cache[get<1>(i.second)]);

        // the actual path to the file
        auto importpath = get<0>(import_cache[get<1>(i.second)]);

        string statement = importname + " " + get<0>(i.second) + ";";
        output = regex_replace(output, regex(i.first), statement);
      }

      string filename;

      if (id != "MODULE1") {
        filename = filepath + ".h";

        /* if (filename[0] == '/') {
          filename = filename.substr(
            fs.cwd().length(), 
            filename.length()
          );
        }
        else if (filename[0] == '.') {
          filename = filename.substr(
            1,
            filename.length()
          );
        }

        filename = "./cc_modules/.build" + filename;
        */
      }
      else {
        filename = filepath;
      }


      filename = path + regex_replace(filename, regex(basePath), "");

      log << log.info << "Writing " << filename << endl;

      ensurePath(filename);
      
      fs.writeFile(filename, output, [&](auto err) {

        if (err) {
          cb(err);
          return;
        }

        if (++index == cache_size) {
          cb(err);
        }
      });
    });
  }
}


int main(int argc, char* argv[]) {

  if (argc < 3) {
    cerr << "Usage: bale <path/to/in.cc> <path/to/out.cc>" << endl;
    exit(1);
  }

  auto die = [=](auto err) {
    if (err.message == "ENOENT") {
      // TODO the message should say what file
      log << log.error << "the file could not be read." << endl;
      exit(1);
    }
    else {
      log << log.error << err.message << endl;
      exit(1);
    }
  };
 
  auto input = p.resolve(fs.cwd(), argv[1]);
  auto output = p.resolve(fs.cwd(), argv[2]);
  auto basePath = p.resolve(fs.cwd(), p.dirname(argv[1]));

  createClassIds(basePath, input, [&](auto err) {
    if (err) die(err);

    rewriteFiles(basePath, output, [&](auto err) {
      if (err) die(err);

    });
  }); 
}

