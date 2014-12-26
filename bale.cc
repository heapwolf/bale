#include <iostream>
#include <string>
#include <regex>
#include <vector>
#include <map>
#include "fs.h"
#include "debug.h"
#include "json.h"

using namespace std;
using namespace fs;
using namespace JSON;

Filesystem filesystem;
Debug log("bale", Debug::verbose, std::cerr);

typedef function<void(fs::Error)> Callback;
typedef vector<const string> Defines;
typedef map<const string, pair<const string, const string> > Imports;

vector<const string> include_cache;
map<const string, const string> import_cache;
int depth = 0;

regex importRE("import\\s+(\\w+)\\s+\"(.*(?!\\/))\";");
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


void createClassIds(const char* path, Callback<fs::Error> cb) {

  filesystem.readFile(path, [&](auto err, auto data) {

    depth++;

    auto imports = find_imports(data);
    int counter = imports.size();

    if (import_cache.find(path) == import_cache.end()) {
      stringstream nextName;

      nextName << "MODULE" << hex << depth << "_H";
      import_cache.insert({ path, nextName.str() });
    }

    if (counter == 0) {
      cb(err);
      return;
    }

    for (auto& import : imports) {

      auto path = get<1>(import.second).c_str();

      createClassIds(path, [&](auto err) {
        counter = counter - 1;
        if (counter == 0) {
          cb(err);
        }
      });
    }

  });
}

void rewriteFiles(const char* path, Callback<fs::Error> cb) {

  int cache_size = import_cache.size();
  int index = 0;

  for (auto& import : import_cache) {

    auto filepath = import.first.c_str();
    filesystem.readFile(filepath, [&](auto err, auto data) {

      auto imports = find_imports(data);

      if (++index < cache_size)
        data = wrap(data, import_cache.at(filepath));

      for (auto& i : imports) {
        auto importpath = import_cache.at(get<1>(i.second));
        auto importname = get<0>(i.second);
        string statement = importpath + " " + importname + ";";
        data = regex_replace(data, regex(i.first), statement);
      }

      string filename = (index < cache_size)
        ? string(filepath) + ".h"
        : string(path)
      ;

      log << log.info << "Writing " << filename << endl;

      filesystem.writeFile(filename.c_str(), data, [&](auto err) {

        if (err) {
          cb(err);
          return;
        }

        if (index == cache_size) {
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
      log << log.error << "the file " << argv[1] << " could not be read." << endl;
      exit(1);
    }
    else {
      log << log.error << err.message << endl;
      exit(1);
    }
  };

  createClassIds(argv[1], [&](auto err) {
    if (err) die(err);
    rewriteFiles(argv[2], [&](auto err) {
      if (err) die(err);
    });
  }); 
}

