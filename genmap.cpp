/* genmap.cpp
Uses libclang to parse a STL header into a local namespace bind
(C) 2014 Niall Douglas http://www.nedproductions.biz/
Created: Sept 2014
*/

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

#include "clang-c/Index.h"
struct index_ptr_deleter { void operator()(CXIndex i){ clang_disposeIndex(i); } };
typedef std::unique_ptr<void, index_ptr_deleter> index_ptr;
struct tu_ptr_deleter { void operator()(CXTranslationUnit tu) { clang_disposeTranslationUnit(tu); } };
typedef std::unique_ptr<struct CXTranslationUnitImpl, tu_ptr_deleter> tu_ptr;
inline std::string to_string(CXString s) { const char *cs=clang_getCString(s); std::string ret(cs); clang_disposeString(s); return ret; }
inline std::string to_string(CXType s) { return s.kind==1 ? "class" : to_string(clang_getTypeSpelling(s)); }

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
  std::unordered_multimap<std::string, std::vector<std::string>> types;
};

struct map_tu_params
{
  header_map out;
  std::vector<std::regex> items;
  path_t path;
  bool addto;
  map_tu_params(std::vector<std::regex> _items, path_t _path, bool _addto) : items(std::move(_items)), path(std::move(_path)), addto(_addto) { }
  std::vector<std::string> location;
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
  void dump(std::ostream &s)
  {
    std::string pathupper(path);
    for(auto &i : pathupper)
      i=std::toupper(i);
    s << "/* This is an automatically generated bindings file. Don't modify it! */" << std::endl;
    s << "#if !defined(BOOST_" << pathupper << "_MAP_START_NAMESPACE) || !defined(BOOST_" << pathupper << "_MAP_END_NAMESPACE)" << std::endl;
    s << "#error You need to define BOOST_" << pathupper << "_MAP_START_NAMESPACE and BOOST_" << pathupper << "_MAP_END_NAMESPACE to use this header file" << std::endl;
    s << "#endif" << std::endl;
    s << "#include <" << path << ">" << std::endl;
    s << "BOOST_" << pathupper << "_MAP_START_NAMESPACE" << std::endl;
    
    for(auto &i : out.types)
    {
      if(!i.second.empty())
      {
        s << "template<";
        bool first=true;
        for(auto &p : i.second)
          s << (first ? (first=false, "") : ", ") << p;
        s << "> ";
      }
      s << "using " << i.first << ";" << std::endl;
    }
    
    s << "BOOST_" << pathupper << "_MAP_END_NAMESPACE" << std::endl;
    s << "#undef BOOST_" << pathupper << "_MAP_START_NAMESPACE" << std::endl;
    s << "#undef BOOST_" << pathupper << "_MAP_END_NAMESPACE" << std::endl;
  }
};
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
    const char *args[]={ "-x", "c++", "-std=c++11" };
    tu=tu_ptr(clang_createTranslationUnitFromSourceFile(index.get(), "__temp.cpp", sizeof(args)/sizeof(args[0]), args, 0, nullptr));
  }
  clang_visitChildren(clang_getTranslationUnitCursor(tu.get()), [](CXCursor cursor, CXCursor parent, CXClientData client_data){
    map_tu_params *p=(map_tu_params *) client_data;
    switch(cursor.kind)
    {
      case CXCursor_Namespace:
        if(!p->location.empty())
          p->location.pop_back();
        //std::cout << "I see namespace " << p->fullqual(to_string(clang_getCursorDisplayName(cursor))) << std::endl;
        auto name(to_string(clang_getCursorSpelling(cursor)));
        p->location.push_back(name);
        for(auto &i : p->items)
        {
          clang_visitChildren(cursor, [](CXCursor cursor, CXCursor parent, CXClientData client_data){
            map_tu_params *p=(map_tu_params *) client_data;
            switch(cursor.kind)
            {
              case CXCursor_UnexposedDecl:
              case CXCursor_StructDecl:
              case CXCursor_UnionDecl:
              case CXCursor_ClassDecl:
              case CXCursor_EnumDecl:
              case CXCursor_ClassTemplate:
                std::cout << "I see entity " << cursor.kind << " " << p->fullqual(to_string(clang_getCursorDisplayName(cursor))) << std::endl;
                auto name(to_string(clang_getCursorSpelling(cursor)));
                auto fullname(p->fullqual(name));
                for(auto &i : p->items)
                  if(std::regex_match(fullname, i))
                  {
                    // This matches
                    std::vector<std::string> params;
                    switch(cursor.kind)
                    {
                      case CXCursor_UnionDecl:
                        break;
                      case CXCursor_ClassTemplate:
                        clang_visitChildren(cursor, [](CXCursor cursor, CXCursor parent, CXClientData client_data){
                          std::vector<std::string> &params=*(std::vector<std::string> *) client_data;
                          auto name(to_string(clang_getCursorType(cursor)));
                          switch(cursor.kind)
                          {
                            case CXCursor_TemplateTypeParameter:
                            case CXCursor_NonTypeTemplateParameter:
                            case CXCursor_TemplateTemplateParameter:
                              std::cout << "I see parameter " << cursor.kind << " of type " << clang_getCursorType(cursor).kind << std::endl;
                              params.push_back(name);
                            break;
                          }
                          return CXChildVisit_Continue;
                        }, &params);
                        // fall through
                      case CXCursor_StructDecl:
                      case CXCursor_ClassDecl:
                      {
                        auto it=p->out.types.equal_range(fullname);
                        // If he's already in there as a template type, and I am not template, skip
                        if(it.first!=p->out.types.end() && !it.first->second.empty() && params.empty())
                          break;
                        bool done=false;
                        for(auto n=it.first; n!=it.second; ++n)
                          if(n->second==params)
                          {
                            done=true;
                            break;
                          }
                        if(!done) p->out.types.insert(std::make_pair(fullname, params));
                        break;
                      }
                      case CXCursor_EnumDecl:
                        p->out.types.insert(std::make_pair(fullname, std::vector<std::string>()));
                        p->location.push_back(name);
                        auto unlocation=undoer([&]{ p->location.pop_back(); });
                        clang_visitChildren(cursor, [](CXCursor cursor, CXCursor parent, CXClientData client_data){
                          map_tu_params *p=(map_tu_params *) client_data;
                          auto name(p->fullqual(to_string(clang_getCursorSpelling(cursor))));
                          /*switch(cursor.kind)
                          {
                            case CXCursor_EnumDecl:*/
                              p->out.types.insert(std::make_pair(name, std::vector<std::string>()));
                          /*  break;
                          }*/
                          return CXChildVisit_Continue;
                        }, p);
                        break;
                    };
                    break;
                  }
                break;
            }
            return CXChildVisit_Continue;
          }, p);
        }
        break;
    }
    return CXChildVisit_Continue;
  }, p);
}

int main(int argc, char *argv[])
{
  std::vector<std::regex> items;
  path_t outpath, inpathdest;
  std::vector<path_t> inpathsrc;
  if(argc<4)
  {
#if 0
    outpath="atomic_map.hpp";
    items.push_back(std::regex("std::[^_].*"));
    inpathdest="atomic";
    //inpathsrc.push_back();
#else
    std::cerr << "Usage: " << argv[0] << " <output> <comma separated list of regexs e.g. std::[^_].*>, <dest header file (can be a system header)> [<src header file>]..." << std::endl;
    return 1;
#endif
  }
  else
  {
    outpath=argv[1];
    inpathdest=argv[3];
    std::cout << "Mapping from header " << inpathdest << " these items:" << std::endl;
    // TODO replace this with regex matching
    for(const char *end=std::strchr(argv[2], 0), *comma=argv[2]-1, *nextcomma;
        nextcomma=comma ? std::strchr(comma+1, ',') : nullptr, comma;
        comma=nextcomma)
    {
      const char *thisend=nextcomma ? nextcomma : end;
      std::cout << "  " << std::string(comma+1, thisend-comma-1) << std::endl;
      items.push_back(std::regex(comma+1, thisend-comma-1));
    }
    for(int n=4; n<argc; n++)
      inpathsrc.push_back(argv[n]);
  }
  map_tu_params tu_p(items, inpathdest, true);
  map_tu(&tu_p);
  {
    std::ofstream o(outpath);
    tu_p.dump(o);
  }
  return 0;
}
