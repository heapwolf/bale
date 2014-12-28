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
map<const string, pair<const string, const string> > import_cache;
int depth = 0;

regex importRE("import\\s+(\\w+)\\s+\"(.*)\";");
regex exportStartRE("export\\s*(\\w+)\\s*\\{");
regex exportEndRE("\\}(\\s*)$");


string wrap(string str, string name) {

  str = "#ifndef " + name + "_h\n#define " + name + "_h\n" + str;
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


string resolveImport(const char* path) {

  bool isPath = path[0] == '/' || path[0] == '.';
  fs::Error pathReference;

  if (isPath) {
    return path;
  }

  string name = string(path);

  auto die = [&]() {
    throw runtime_error("Module not found: " + name);
  };

  function<string(string)> visit;
  visit = [&](string p) {

    string newpath = p + "/cc_modules/" + name + "/index.cc";

    try {
      filesystem.statSync(newpath.c_str());
    }
    catch(...) {

      string parent = p.substr(0, p.find_last_of("/\\"));
      
      if (parent.length() == 0) {
        die();
      }
      return visit(parent);
    }
    return newpath;
  };
  
  return visit(filesystem.cwd());
}


void createClassIds(const char* value) {

  string path = resolveImport(value);
  uv_buf_t buffer;

  try {
    buffer = filesystem.readFileSync(path.c_str());
  }
  catch(...) {
    return;
  }

  string data = string(buffer.base);

  depth++;

  auto imports = find_imports(data);
  auto counter = imports.size();

  if (import_cache.find(value) == import_cache.end()) {

    string id = "module" + to_string(depth);
    import_cache.insert({ value, make_pair(path, id) });
  }

  for (auto& import : imports) {
    auto path = get<1>(import.second).c_str();
    createClassIds(path);
  }
}


void ensurePath(string path) {

  istringstream ss(path.substr(0, path.find_last_of("/\\")));
  string token;
  string tmp;

  while (std::getline(ss, token, '/')) {
    tmp += token + "/";
    try {
      filesystem.statSync(tmp.c_str());
    }
    catch(...) {
      filesystem.mkdirSync(tmp.c_str());
    }
  }
}


void rewriteFiles(const char* path) {

  int cache_size = import_cache.size();
  int index = 0;

  for (auto& import : import_cache) {

    auto filepath = get<0>(import.second);
    auto id = get<1>(import.second);

    //ensurePath(filepath);

    //if(filepath.length() == 0) {
      //cout << path << endl;
    //  continue;
   // }
 
    auto readBuffer = filesystem.readFileSync(filepath.c_str());
    cout << "----" << endl;
    cout << string(readBuffer.base);
    cout << "----" << endl;
    /* string data(readBuffer.base);
    auto imports = find_imports(data);

    if (id != "module1")
      data = wrap(data, get<1>(import.second));

    for (auto& i : imports) {
      // the identity of the new class 
      auto importname = get<1>(import_cache[get<1>(i.second)]);

      // the actual path to the file
      auto importpath = get<0>(import_cache[get<1>(i.second)]);

      string statement = importname + " " + get<0>(i.second) + ";";
      data = regex_replace(data, regex(i.first), statement);
    }

    string filename;

    if (id != "module") {
      filename = string(filepath) + ".h";

      if (filename[0] == '/') {
        filename = filename.substr(
          filesystem.cwd().length(), 
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
    }
    else {
      filename = string(path);
    }

    ensurePath(filename);

    log << log.info << "Writing " << filename << endl;

    //auto writeBuffer = filesystem.createBuffer(data);
 
    try {
      filesystem.writeFileSync(filename.c_str(), data);
    }
    catch(...) {
      return;
    } */
 
    //string cmd = "CC -x c++-header " + filename + " -std=c++1y";
    //auto i = system(cmd.c_str());
  }
} 


int main(int argc, char* argv[]) {

  if (argc < 3) {
    cerr << "Usage: bale <path/to/in.cc> <path/to/out.cc>" << endl;
    exit(1);
  }

  createClassIds(argv[1]);
  rewriteFiles(argv[2]);
}

