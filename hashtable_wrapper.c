#include <stdlib.h>

#include <Python.h>

#include "hashtable.h"

static PyObject *py_table_create(PyObject *self, PyObject *args) {
  size_t columns_len;

  int rc = PyArg_ParseTuple(args, "n", &columns_len);
  if (rc == 0) {
    return NULL;
  }
  struct table *t = table_create(columns_len);
  if (t == NULL) {
    PyErr_SetString(PyExc_RuntimeError, "Failed to create table");
    return NULL;
  }
  return PyCapsule_New(t, "hashtable.table", NULL);
}

static PyObject *py_table_destroy(PyObject *self, PyObject *args) {
  PyObject *py_table;

  int rc = PyArg_ParseTuple(args, "O", &py_table);
  if (rc == 0) {
    return NULL;
  }
  struct table *t = PyCapsule_GetPointer(py_table, "hashtable.table");
  if (t == NULL) {
    PyErr_SetString(PyExc_RuntimeError, "Invalid table capsule");
    return NULL;
  }
  table_destroy(t);
  Py_RETURN_NONE;
}

static PyObject *py_table_put(PyObject *self, PyObject *args) {
  PyObject *py_table, *value;
  const char *key;

  int rc = PyArg_ParseTuple(args, "OsO", &py_table, &key, &value);
  if (rc == 0) {
    return NULL;
  }
  struct table *t = PyCapsule_GetPointer(py_table, "hashtable.table");
  if (t == NULL) {
    PyErr_SetString(PyExc_RuntimeError, "Invalid table capsule");
    return NULL;
  }

  int result = table_put(t, key, (void *)value);
  if (result == 0) {
    // Increment reference count for the Python object to keep it alive
    Py_INCREF(value);
  }
  return PyLong_FromLong(result);
}

static PyObject *py_table_get(PyObject *self, PyObject *args) {
  PyObject *py_table;
  const char *key;

  int rc = PyArg_ParseTuple(args, "Os", &py_table, &key);
  if (rc == 0) {
    return NULL;
  }
  struct table *t = PyCapsule_GetPointer(py_table, "hashtable.table");
  if (t == NULL) {
    PyErr_SetString(PyExc_RuntimeError, "Invalid table capsule");
    return NULL;
  }

  void *result = table_get(t, key);
  if (result == NULL) {
    Py_RETURN_NONE;
  }
  return (PyObject *)result;
}

static PyMethodDef hashtable_methods[] = {
  {"create", py_table_create, METH_VARARGS, "Create a new table"},
  {"destroy", py_table_destroy, METH_VARARGS, "Destroy an existing table"},
  {"put", py_table_put, METH_VARARGS, "Insert a key-value pair into the table"},
  {"get", py_table_get, METH_VARARGS, "Retrieve a value by key from the table"},
  {NULL, NULL, 0, NULL},
};

static struct PyModuleDef hashtable_module = {
  PyModuleDef_HEAD_INIT,
  "hashtable",
  NULL,
  -1,
  hashtable_methods,
};

PyMODINIT_FUNC PyInit_hashtable(void) {
  return PyModule_Create(&hashtable_module);
}
