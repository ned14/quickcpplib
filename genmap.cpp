/* genmap.cpp
Uses libclang to parse a STL header into a local namespace bind
(C) 2014 Niall Douglas http://www.nedproductions.biz/
Created: Sept 2014
*/

#define LOGGING 0

#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>
#include <utility>
#include <unordered_map>
#include <locale>
#include <regex>

#define __STDC_LIMIT_MACROS 1
#define __STDC_CONSTANT_MACROS 1
#include "clang-c/Index.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Type.h"

struct index_ptr_deleter { void operator()(CXIndex i){ clang_disposeIndex(i); } };
typedef std::unique_ptr<void, index_ptr_deleter> index_ptr;
struct tu_ptr_deleter { void operator()(CXTranslationUnit tu) { clang_disposeTranslationUnit(tu); } };
typedef std::unique_ptr<struct CXTranslationUnitImpl, tu_ptr_deleter> tu_ptr;
inline std::string to_string(CXString s) { const char *cs=clang_getCString(s); std::string ret(cs); clang_disposeString(s); return ret; }
inline std::string to_string(CXType s) { return /*s.kind==1 ? "class" :*/ to_string(clang_getTypeSpelling(s)); }
inline std::string to_identifier(std::string i) { for(size_t n=0; n<i.size(); n++) if(i[n]=='-') i[n]='_'; return i; }

typedef std::string path_t;

namespace Impl {
  template<typename T, bool iscomparable> struct is_nullptr { bool operator()(T c) const { return !c; } };
  template<typename T> struct is_nullptr<T, false> { bool operator()(T) const { return false; } };
}
#if defined(__GNUC__) && ((__GNUC__*10000 + __GNUC_MINOR__*100 + __GNUC_PATCHLEVEL__ <50000) || defined(__MINGW32__))
template<typename T> bool is_nullptr(T v) { return Impl::is_nullptr<T, std::is_constructible<bool, T>::value>()(std::forward<T>(v)); }
#else
template<typename T> bool is_nullptr(T v) { return Impl::is_nullptr<T, std::is_trivially_constructible<bool, T>::value>()(std::forward<T>(v)); }
#endif


template<typename callable> class UndoerImpl
{
  bool _dismissed;
  callable undoer;
  UndoerImpl() = delete;
  UndoerImpl(const UndoerImpl &) = delete;
  UndoerImpl &operator=(const UndoerImpl &) = delete;
  explicit UndoerImpl(callable &&c) : _dismissed(false), undoer(std::move(c)) { }
  void int_trigger() { if(!_dismissed && !is_nullptr(undoer)) { undoer(); _dismissed=true; } }
public:
  UndoerImpl(UndoerImpl &&o) : _dismissed(o._dismissed), undoer(std::move(o.undoer)) { o._dismissed=true; }
  UndoerImpl &operator=(UndoerImpl &&o) { int_trigger(); _dismissed=o._dismissed; undoer=std::move(o.undoer); o._dismissed=true; return *this; }
  template<typename _callable> friend UndoerImpl<_callable> undoer(_callable c);
  ~UndoerImpl() { int_trigger(); }
  //! Returns if the Undoer is dismissed
  bool dismissed() const { return _dismissed; }
  //! Dismisses the Undoer
  void dismiss(bool d=true) { _dismissed=d; }
  //! Undismisses the Undoer
  void undismiss(bool d=true) { _dismissed=!d; }
};//UndoerImpl


/*! \brief Alexandrescu style rollbacks, a la C++ 11.

Example of usage:
\code
auto resetpos=Undoer([&s]() { s.seekg(0, std::ios::beg); });
...
resetpos.dismiss();
\endcode
*/
template<typename callable> inline UndoerImpl<callable> undoer(callable c)
{
  //static_assert(!std::is_function<callable>::value && !std::is_member_function_pointer<callable>::value && !std::is_member_object_pointer<callable>::value && !has_call_operator<callable>::value, "Undoer applied to a type not providing a call operator");
  auto foo=UndoerImpl<callable>(std::move(c));
  return foo;
}//Undoer

struct header_map
{
  std::unordered_map<std::string, std::vector<std::string>> enums; // enum name: values
  std::unordered_multimap<std::string, std::pair<bool, std::vector<std::string>>> types; // type name: alias, template parameters
};

struct map_tu_params
{
  header_map out;
  std::vector<std::regex> items;
  path_t path;
  map_tu_params(std::vector<std::regex> _items, path_t _path) : items(std::move(_items)), path(std::move(_path)), scratch(nullptr) { }
  std::vector<std::string> location;
  void *scratch;
  std::string fullqual(std::string id) const
  {
    std::string ret;
    for(auto &i : location)
    {
      if(!ret.empty()) ret.append("::");
      ret.append(i);
    }
    if(!ret.empty()) ret.append("::");
    ret.append(id);
    return ret;
  }
  std::string leafname(std::string fullname)
  {
    std::smatch results;
    for(auto &i : items)
    {
      if(std::regex_match(fullname, results, i))
      {
#if LOGGING && 0
        std::cout << "Matched " << results.size() << " items:";
        for(auto &m : results)
          std::cout << " " << m;
        std::cout << std::endl;
#endif
        return results[results.size()-1].str();
      }
    }
    return fullname;
  }
  void leafname(std::string &namespace1, std::string &leaf, std::string &namespace2, std::string fullname)
  {
    std::smatch results;
    namespace1.clear();
    namespace2.clear();
    for(auto &i : items)
    {
      if(std::regex_match(fullname, results, i))
      {
#if LOGGING && 0
        std::cout << "Matched " << results.size() << " items:";
        for(auto &m : results)
          std::cout << " " << m;
        std::cout << std::endl;
#endif
        std::string match=results[results.size()-1].str();
        for(size_t colon; ((colon=match.find("::"))!=std::string::npos);)
        {
          namespace1.append("namespace "+match.substr(0, colon)+" { ");
          namespace2.append(" }");
          match=match.substr(colon+2);
        }
        leaf=match;
        return;
      }
    }
    leaf=fullname;
  }
  void filter(map_tu_params &o)
  {
    std::vector<std::string> enums, types;
    for(auto &i : out.enums)
    {
      std::string l(leafname(i.first));
#if LOGGING
      std::cout << "Checking " << l << std::endl;
#endif
      bool found=false;
      for(auto &m : o.out.enums)
        if(l==o.leafname(m.first))
        {
          found=true;
          break;
        }
      if(!found)
      {
        enums.push_back(i.first);
#if LOGGING
        std::cout << "Removing " << i.first << " as not in all input headers.\n";
#endif
      }
    }
    for(auto &i : enums)
      out.enums.erase(i);
    for(auto &i : out.types)
    {
      std::string l(leafname(i.first));
#if LOGGING
      std::cout << "Checking " << l << std::endl;
#endif
      bool found=false;
      for(auto &m : o.out.types)
        if(l==o.leafname(m.first))
        {
          found=true;
          break;
        }
      if(!found)
      {
        types.push_back(i.first);
#if LOGGING
        std::cout << "Removing " << i.first << " as not in all input headers.\n";
#endif
      }
    }
    for(auto &i : types)
      out.types.erase(i);      
  }
  void dump(std::ostream &s, std::string macro_prefix)
  {
    std::string pathupper(path), namespace1, leaf, namespace2;
    for(auto &i : pathupper)
      i=std::toupper(i);
    s << "/* This is an automatically generated bindings file. Don't modify it! */" << std::endl;
    s << "#if !defined(" << macro_prefix << "NAMESPACE_BEGIN) || !defined(" << macro_prefix << "NAMESPACE_END)" << std::endl;
    s << "#error You need to define " << macro_prefix << "NAMESPACE_BEGIN and " << macro_prefix << "NAMESPACE_END to use this header file" << std::endl;
    s << "#endif" << std::endl;
//    s << "#include \"boostmacros.hpp\"\n";
    s << "#include <" << path << ">" << std::endl;
    s << macro_prefix << "NAMESPACE_BEGIN" << std::endl;
    s << "extern const char *boost_local_bind_in;" << std::endl;

    // Enums before all else   
    for(auto &i : out.enums)
    {
      leafname(namespace1, leaf, namespace2, i.first);
      s << namespace1 << "using " << i.first << ((i.second.empty() || namespace2.empty()) ? ";\n" : ";");
      for(auto &v : i.second)
        s << "  using " << v << ";\n";
      if(!namespace2.empty())
        s << namespace2 << std::endl;
    }
    // Map out each type, template aliased where appropriate
    for(auto &i : out.types)
    {
      leafname(namespace1, leaf, namespace2, i.first);
      std::string leafupper(leaf);
      for(auto &i : leafupper)
        i=std::toupper(i);
      bool isAlias=i.second.first;
      auto &templatepars=i.second.second;
      s << "#ifdef " << macro_prefix << "NO_" << leafupper << std::endl;
      s << "#undef " << macro_prefix << "NO_" << leafupper << std::endl;
      s << "#else" << std::endl;
      s << namespace1;
      if(!templatepars.empty())
      {
        s << "template<";
        bool first=true;
        for(auto &p : templatepars)
          s << (first ? (first=false, "") : ", ") << p;
        s << "> ";
      }
      s << "using ";
      if(isAlias)
        s << leaf << " = ";
      s << i.first;
      if(!templatepars.empty())
      {
        s << "<";
        bool first=true;
        for(auto &p : templatepars)
        {
          s << (first ? (first=false, "") : ", ") << &p[p.rfind(" ")+1];
          if(p.find("...")!=std::string::npos)
            s << "...";
        }
        s << ">";
      }
      s << ";" << namespace2 << std::endl;
      s << "#endif" << std::endl;
    }
    
    s << macro_prefix << "NAMESPACE_END" << std::endl;
    s << "#undef " << macro_prefix << "NAMESPACE_BEGIN" << std::endl;
    s << "#undef " << macro_prefix << "NAMESPACE_END" << std::endl;
  }
};
static void parse_namespace(CXCursor cursor, map_tu_params *p)
{
  //std::cout << "I see namespace " << p->fullqual(to_string(clang_getCursorDisplayName(cursor))) << std::endl;
  auto name(to_string(clang_getCursorSpelling(cursor)));
  p->location.push_back(name);
  auto unlocation=undoer([p]{p->location.pop_back();});
  for(auto &i : p->items)
  {
    clang_visitChildren(cursor, [](CXCursor cursor, CXCursor parent, CXClientData client_data){
      map_tu_params *p=(map_tu_params *) client_data;
      switch(cursor.kind)
      {
        case CXCursor_Namespace:
          // Recurse
          parse_namespace(cursor, p);
          break;
        case CXCursor_StructDecl:
        case CXCursor_ClassDecl:
        case CXCursor_EnumDecl:
        case CXCursor_FunctionDecl:
        case CXCursor_TypedefDecl:
        case CXCursor_FunctionTemplate:
        case CXCursor_ClassTemplate:
        case CXCursor_TypeAliasDecl:
        {
          bool isAlias=false;
          auto name(to_string(clang_getCursorSpelling(cursor)));
          // Skip operator mapping as ADL will find those.
          if(name.compare(0, 8, "operator") == 0)
            break;
          // If the semantic parent is not a namespace, this is an out of class implementation. Skip
          if(clang_getCursorSemanticParent(cursor).kind!=CXCursor_Namespace)
            break;
#if LOGGING
          std::cout << "I see entity " << cursor.kind << " " << p->fullqual(to_string(clang_getCursorDisplayName(cursor))) << " [" << name << "]" << std::endl;
          //std::cout << "*** parent kind = " << parent.kind << " semantic parent kind = " << clang_getCursorSemanticParent(cursor).kind << std::endl;
#endif
          auto fullname(p->fullqual(name));
          for(auto &i : p->items)
            if(std::regex_match(fullname, i))
            {
              // This matches
              std::vector<std::string> params;
              switch(cursor.kind)
              {
                case CXCursor_ClassTemplate:
                  clang_visitChildren(cursor, [](CXCursor cursor, CXCursor parent, CXClientData client_data){
                    std::vector<std::string> &params=*(std::vector<std::string> *) client_data;
                    CXType type=clang_getCursorType(cursor);
                    auto name(to_string(type));
                    CXCursor param_decl=clang_getTypeDeclaration(type);
                    switch(cursor.kind)
                    {
                      case CXCursor_TemplateTemplateParameter:
                      case CXCursor_TemplateTypeParameter:
                      {
                        /* libclang also seems incapable of telling us whether a type is a variadic
                          * pack, so once again drop into the C++ API ...
                          * type[2]= { QualType_as_opaque_ptr, TU };
                          */
                        bool isPack=false;
                        {
                          using namespace clang;
                          QualType T = QualType::getFromOpaquePtr(type.data[0]);
                          if (!T.isNull())
                          {
                            isPack=T->containsUnexpandedParameterPack();
                          }
                        }
                        std::string prefix("class");
                        if(isPack)
                          prefix.append("...");
                        name=prefix+" "+to_identifier(name);
#if LOGGING
                        std::cout << "I see parameter " << cursor.kind << " of type " << type.kind << " isPack=" << isPack << std::endl;
#endif
                        params.push_back(name);
                        break;
                      }
                      case CXCursor_NonTypeTemplateParameter:
#if LOGGING
                        std::cout << "I see parameter " << cursor.kind << " of type " << type.kind << std::endl;
#endif
                        name.append(" _"+std::to_string(params.size()));
                        params.push_back(name);
                        break;
                      default:
#if LOGGING
                        std::cout << "I see unknown parameter " << cursor.kind << " of type " << type.kind << std::endl;
#endif
                        break;
                    }
                    return CXChildVisit_Continue;
                  }, &params);
                  // fall through
                case CXCursor_StructDecl:
                case CXCursor_ClassDecl:
                  isAlias=true;
                  // fall through
                case CXCursor_FunctionDecl:
                case CXCursor_TypedefDecl:
                case CXCursor_FunctionTemplate:
                case CXCursor_TypeAliasDecl:
                {
                  auto it=p->out.types.equal_range(fullname);
                  // If he's already in there as a template type, and I am not template, skip
                  if(it.first!=p->out.types.end() && !it.first->second.second.empty() && params.empty())
                    break;
                  bool done=false;
                  for(auto n=it.first; n!=it.second; ++n)
                    if(n->second.second==params)
                    {
                      done=true;
                      break;
                    }
                  if(!done) p->out.types.insert(std::make_pair(std::move(fullname), std::make_pair(isAlias, std::move(params))));
                  break;
                }
                case CXCursor_EnumDecl:
                  /* libclang won't tell us whether this is a scoped enum or not, so ...
                    * A CXCursor contains a kind, an int xdata member, and a void *data[3]
                    * data[3] = { const clang::Decl *Parent, const clang::Stmt *S, CXTranslationUnit TU };
                    * 
                    * This function is implemented like this:
                    *  CXType clang_getEnumDeclIntegerType(CXCursor C) {
                    *    using namespace cxcursor;
                    *    CXTranslationUnit TU = cxcursor::getCursorTU(C);
                    *    if (clang_isDeclaration(C.kind)) {
                    *      const Decl *D = static_cast<const Decl *>(C.data[0]);
                    *      if (const EnumDecl *TD = dyn_cast_or_null<EnumDecl>(D)) {
                    *        QualType T = TD->getIntegerType();
                    *        return MakeCXType(T, TU);
                    *      }
                    *      return MakeCXType(QualType(), TU);
                    *    }
                    *    return MakeCXType(QualType(), TU);
                    *  }
                    */
                  bool isScoped=false;
                  {
                    using namespace clang;
                    const Decl *D = static_cast<const Decl *>(cursor.data[0]);
                    if(const EnumDecl *TD = dyn_cast_or_null<EnumDecl>(D))
                    {
                      isScoped=TD->isScoped();
                    }
                  }
#if LOGGING
                  std::cout << "I see enum " << cursor.kind << " of type " << clang_getCursorType(cursor).kind << " isScoped=" << isScoped << std::endl;
#endif
                  auto enumit=p->out.enums.insert(std::make_pair(fullname, std::vector<std::string>())).first;
                  // If scoped we don't need to duplicate binds into the surrounding namespace
                  if(isScoped)
                    break;
                  typedef decltype(enumit) enumit_t;
                  p->scratch=&enumit;
                  //p->location.push_back(name);
                  //auto unlocation=undoer([&]{ p->location.pop_back(); });
                  clang_visitChildren(cursor, [](CXCursor cursor, CXCursor parent, CXClientData client_data){
                    map_tu_params *p=(map_tu_params *) client_data;
                    enumit_t &enumit=*(enumit_t *) p->scratch;
                    auto name(p->fullqual(to_string(clang_getCursorSpelling(cursor))));
                    /*switch(cursor.kind)
                    {
                      case CXCursor_EnumDecl:*/
                        enumit->second.push_back(name);
                    /*  break;
                    }*/
                    return CXChildVisit_Continue;
                  }, p);
                  break;
              }
              break;
            }
          break;
        }
      }
      return CXChildVisit_Continue;
    }, p);
  }
}
void map_tu(map_tu_params *p)
{
  index_ptr index(clang_createIndex(1,1));
  tu_ptr tu;
  {
    auto untempfile=undoer([]{ std::remove("__temp.cpp"); });
    {
      std::ofstream temph("__temp.cpp");  
      temph << "#include <" << p->path << ">" << std::endl;
    }
#if 0
    const char *args[]={ "-x", "c++", "-std=c++11", "-I." };
#else
    //const char *args[]={ "-x", "c++", "-std=c++11", "-nostdinc++", "-I/usr/include/c++/4.8", "-I/usr/include/x86_64-linux-gnu/c++/4.8" };
    const char *args[]={ "-x", "c++", "-std=c++11", "-I.", "-I/home/ned/boost-release" };
#endif
    tu=tu_ptr(clang_createTranslationUnitFromSourceFile(index.get(), "__temp.cpp", sizeof(args)/sizeof(args[0]), args, 0, nullptr));
    clang_visitChildren(clang_getTranslationUnitCursor(tu.get()), [](CXCursor cursor, CXCursor parent, CXClientData client_data){
      map_tu_params *p=(map_tu_params *) client_data;
      switch(cursor.kind)
      {
        case CXCursor_Namespace:
          parse_namespace(cursor, p);
      }
      return CXChildVisit_Continue;
    }, p);
  }
}

int main(int argc, char *argv[])
{
  path_t outpath;
  std::string prefix;
  std::vector<std::pair<std::vector<std::regex>, path_t>> inpaths;
  if(argc<4 || (argc&1)!=1)
  {
#if 0
    outpath="atomic_map.hpp";
    items.push_back(std::regex("std::[^_].*"));
    inpathdest="atomic";
    //inpathsrc.push_back();
#else
    std::cerr << "Usage: " << argv[0] << " <output> <macro prefix> <comma separated list of regexs e.g. std::[^_].*>, <dest header file (can be a system header)> [<comma separated list of regexs> <src header file>]..." << std::endl;
    return 1;
#endif
  }
  else
  {
    outpath=argv[1];
    prefix=argv[2];
    auto parse_regex=[](const char *argv){
      // TODO replace this with regex matching
      std::vector<std::regex> items;
      for(const char *end=std::strchr(argv, 0), *comma=argv-1, *nextcomma;
          nextcomma=comma ? std::strchr(comma+1, ',') : nullptr, comma;
          comma=nextcomma)
      {
        const char *thisend=nextcomma ? nextcomma : end;
        std::cout << "  " << std::string(comma+1, thisend-comma-1) << std::endl;
        items.push_back(std::regex(comma+1, thisend-comma-1));
      }
      return items;
    };
    for(int n=3; n<argc; n+=2)
    {
      std::cout << "Mapping from header " << argv[n+1] << " these items:" << std::endl;
      inpaths.push_back(std::make_pair(parse_regex(argv[n]), argv[n+1]));
    }
  }
  // Add the contents of the first header
  map_tu_params tu_p(inpaths[0].first, inpaths[0].second);
  map_tu(&tu_p);
  // Subtract anything not in the contents of any further headers
  for(size_t n=1; n<inpaths.size(); n++)
  {
    map_tu_params tu2_p(inpaths[n].first, inpaths[n].second);
    map_tu(&tu2_p);
    tu_p.filter(tu2_p);
  }
  {
    std::ofstream o(outpath);
    tu_p.dump(o, prefix);
  }
  return 0;
}
