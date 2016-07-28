#include <Python.h>

typedef struct {
	PyObject_HEAD
	PyObject *object;
} Reference;

static int Reference_init(Reference *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = {"obj", NULL};

	if (!__atomic_is_lock_free(sizeof(self->object), &self->object)) {
		if (PyErr_WarnEx(PyExc_RuntimeWarning,
				 "atomic.Reference is not lock free", 1) < 0)
			return -1;
	}

	self->object = Py_None;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist,
					 &self->object))
		return -1;

	Py_INCREF(self->object);

	return 0;
}

static int Reference_traverse(Reference *self, visitproc visit, void *arg)
{
	PyObject *object;

	__atomic_load(&self->object, &object, __ATOMIC_SEQ_CST);
	
	Py_VISIT(object);
	return 0;
}

static int Reference_clear(Reference *self)
{
	PyObject *object;

	object = __atomic_exchange_n(&self->object, NULL, __ATOMIC_SEQ_CST);

	Py_XDECREF(object);
	return 0;
}

static void Reference_dealloc(Reference *self)
{
	Reference_clear(self);
	Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *Reference_repr(Reference *self)
{
	PyObject *object;

	__atomic_load(&self->object, &object, __ATOMIC_SEQ_CST);

	return PyUnicode_FromFormat("atomic.Reference(%R)", object);
}

static PyObject *Reference_get(Reference *self)
{
	PyObject *object;

	__atomic_load(&self->object, &object, __ATOMIC_SEQ_CST);

	Py_INCREF(object);
	return object;
}

static PyObject *Reference_set(Reference *self, PyObject *args)
{
	PyObject *object, *old_object;

	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;

	Py_INCREF(object);

	__atomic_exchange(&self->object, &object, &old_object,
			  __ATOMIC_SEQ_CST);

	Py_DECREF(old_object);
	Py_RETURN_NONE;
}

static PyObject *Reference_get_and_set(Reference *self, PyObject *args)
{
	PyObject *object, *ret;

	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;

	Py_INCREF(object);

	__atomic_exchange(&self->object, &object, &ret, __ATOMIC_SEQ_CST);

	return ret;
}

static PyObject *Reference_compare_and_set(Reference *self, PyObject *args)
{
	PyObject *expect, *update;
	long ret;

	if (!PyArg_ParseTuple(args, "OO", &expect, &update))
		return NULL;

	Py_INCREF(update);

	ret = __atomic_compare_exchange(&self->object, &expect, &update, 0,
					__ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);

	if (!ret)
		Py_DECREF(update);
	Py_DECREF(expect);
	return PyBool_FromLong(ret);
}

static PyObject *Reference_weak_compare_and_set(Reference *self, PyObject *args)
{
	PyObject *expect, *update;
	long ret;

	if (!PyArg_ParseTuple(args, "OO", &expect, &update))
		return NULL;

	Py_INCREF(update);

	ret = __atomic_compare_exchange(&self->object, &expect, &update, 1,
					__ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);

	if (!ret)
		Py_DECREF(update);
	Py_DECREF(expect);
	return PyBool_FromLong(ret);
}

static PyMethodDef Reference_methods[] = {
	{"get", (PyCFunction)Reference_get, METH_NOARGS,
	 "get() -> object\n\n"
	 "Atomically load and return the stored reference."},
	{"set", (PyCFunction)Reference_set, METH_VARARGS,
	 "set(obj)\n\n"
	 "Atomically store the given reference."},

	{"get_and_set", (PyCFunction)Reference_get_and_set, METH_VARARGS,
	 "get_and_set(x) -> object\n\n"
	 "Atomically store the given reference and return the old reference."},
	{"compare_and_set", (PyCFunction)Reference_compare_and_set, METH_VARARGS,
	 "compare_and_set(expect, update) -> bool\n\n"
	 "Atomically store the given reference if the old reference equals the given\n"
	 "expected reference by identity, returning whether the actual reference equaled\n"
	 "the expected value."},
	{"weak_compare_and_set", (PyCFunction)Reference_weak_compare_and_set, METH_VARARGS,
	 "weak_compare_and_set(expect, update) -> bool\n\n"
	 "compare_and_set, but can fail spuriously and does not provide ordering\n"
	 "guarantees."},

	{NULL, NULL, 0, NULL}
};

#define ATOMIC_REFERENCE_DOCSTRING \
	"atomic.Reference(obj=None) -> new atomic reference\n\n" \
	"Reference supporting atomic operations with sequentially consistent semantics.\n\n" \
	"Atomic load (get()), store (set()), exchange (get_and_set()),\n" \
	"and compare-and-exchange (compare_and_set()) are supported."

PyTypeObject Reference_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"atomic.Reference",			/* tp_name */
	sizeof(Reference),			/* tp_basicsize */
	0,					/* tp_itemsize */
	(destructor)Reference_dealloc,		/* tp_dealloc */
	NULL,					/* tp_print */
	NULL,					/* tp_getattr */
	NULL,					/* tp_setattr */
	NULL,					/* tp_reserved */
	(reprfunc)Reference_repr,		/* tp_repr */
	NULL,					/* tp_as_number */
	NULL,					/* tp_as_sequence */
	NULL,					/* tp_as_mapping */
	NULL,					/* tp_hash  */
	NULL,					/* tp_call */
	NULL,					/* tp_str */
	NULL,					/* tp_getattro */
	NULL,					/* tp_setattro */
	NULL,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,			/* tp_flags */
	ATOMIC_REFERENCE_DOCSTRING,		/* tp_doc */
	(traverseproc)Reference_traverse,	/* tp_traverse */
	(inquiry)Reference_clear,		/* tp_clear */
	NULL,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	NULL,					/* tp_iter */
	NULL,					/* tp_iternext */
	Reference_methods,			/* tp_methods */
	NULL,					/* tp_members */
	NULL,					/* tp_getset */
	NULL,					/* tp_base */
	NULL,					/* tp_dict */
	NULL,					/* tp_descr_get */
	NULL,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	(initproc)Reference_init,		/* tp_init */
};
