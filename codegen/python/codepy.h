#ifndef CODEPY_H
#define CODEPY_H

#include "parser/parser.h"
#include "utils/utils.h"

#define MAX_PY_FUNCS 64
#define MAX_PY_PARAMS 8

#define AWL_PY_RUNTIME \
"#include <Python.h>\n" \
"#include <stdarg.h>\n" \
"#include <string.h>\n\n" \
"static int _py_ready = 0;\n" \
"static char _py_str_buf[4096];\n" \
"static PyObject *_py_namespaces[32];\n" \
"static char _py_names[32][64];\n" \
"static int _py_ns_count = 0;\n\n" \
"static void _py_init(void){if(_py_ready)return;Py_Initialize();_py_ready=1;}\n" \
"static void _py_load(const char*path,const char*mod){\n" \
"    _py_init();\n" \
"    FILE*f=fopen(path,\"r\");if(!f){fprintf(stderr,\"Cannot open: %s\\n\",path);return;}\n" \
"    PyObject*ns=PyDict_New();\n" \
"    PyDict_SetItemString(ns,\"__builtins__\",PyEval_GetBuiltins());\n" \
"    PyObject*r=PyRun_FileEx(f,path,Py_file_input,ns,ns,1);\n" \
"    if(!r)PyErr_Print();else Py_DECREF(r);\n" \
"    _py_namespaces[_py_ns_count]=ns;\n" \
"    strncpy(_py_names[_py_ns_count],mod,63);\n" \
"    _py_ns_count++;\n" \
"}\n" \
"static PyObject*_py_ns(const char*mod){\n" \
"    for(int i=0;i<_py_ns_count;i++)if(strcmp(_py_names[i],mod)==0)return _py_namespaces[i];\n" \
"    fprintf(stderr,\"Module '%s' not found\\n\",mod);return NULL;\n" \
"}\n" \
"static int _py_call_int(const char*mod,const char*func,int argc,...){\n" \
"    PyObject*ns=_py_ns(mod);if(!ns)return 0;\n" \
"    PyObject*pf=PyDict_GetItemString(ns,func);if(!pf){fprintf(stderr,\"Func '%s' not found\\n\",func);return 0;}\n" \
"    va_list ap;va_start(ap,argc);\n" \
"    PyObject*args=PyTuple_New(argc);\n" \
"    for(int i=0;i<argc;i++)PyTuple_SetItem(args,i,PyLong_FromLong(va_arg(ap,int)));\n" \
"    va_end(ap);\n" \
"    PyObject*r=PyObject_CallObject(pf,args);Py_DECREF(args);\n" \
"    if(!r){PyErr_Print();return 0;}\n" \
"    int ret=(int)PyLong_AsLong(r);Py_DECREF(r);return ret;\n" \
"}\n" \
"static float _py_call_float(const char*mod,const char*func,int argc,...){\n" \
"    PyObject*ns=_py_ns(mod);if(!ns)return 0.0f;\n" \
"    PyObject*pf=PyDict_GetItemString(ns,func);if(!pf)return 0.0f;\n" \
"    va_list ap;va_start(ap,argc);\n" \
"    PyObject*args=PyTuple_New(argc);\n" \
"    for(int i=0;i<argc;i++)PyTuple_SetItem(args,i,PyFloat_FromDouble(va_arg(ap,double)));\n" \
"    va_end(ap);\n" \
"    PyObject*r=PyObject_CallObject(pf,args);Py_DECREF(args);\n" \
"    if(!r){PyErr_Print();return 0.0f;}\n" \
"    float ret=(float)PyFloat_AsDouble(r);Py_DECREF(r);return ret;\n" \
"}\n" \
"static const char*_py_call_str(const char*mod,const char*func,int argc,...){\n" \
"    PyObject*ns=_py_ns(mod);if(!ns)return \"\";\n" \
"    PyObject*pf=PyDict_GetItemString(ns,func);if(!pf)return \"\";\n" \
"    va_list ap;va_start(ap,argc);\n" \
"    PyObject*args=PyTuple_New(argc);\n" \
"    for(int i=0;i<argc;i++){const char*s=va_arg(ap,const char*);PyTuple_SetItem(args,i,PyUnicode_FromString(s));}\n" \
"    va_end(ap);\n" \
"    PyObject*r=PyObject_CallObject(pf,args);Py_DECREF(args);\n" \
"    if(!r){PyErr_Print();return \"\";}\n" \
"    const char*ret=PyUnicode_AsUTF8(r);\n" \
"    if(ret)strncpy(_py_str_buf,ret,sizeof(_py_str_buf)-1);else _py_str_buf[0]='\\0';\n" \
"    Py_DECREF(r);return _py_str_buf;\n" \
"}\n"

typedef struct {
    char *name;
    DataType returnType;   
    DataType paramTypes[MAX_PY_PARAMS];
    int paramCount;
} PyFunc;

typedef struct {
    char *name; 
    PyFunc funcs[MAX_PY_FUNCS];
    int count;
} PyModule;

extern PyModule pyModules[128];
extern int pyModuleCount;
extern char pyModuleNames[16][64];

PyModule parsePyFile(const char *path);
PyFunc *findPyFunc(const char *modname, const char *funcname);

#endif