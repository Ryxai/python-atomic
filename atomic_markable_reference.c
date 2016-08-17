#include <Python.h>
#define PyBool_ExcCheck(bool, message) if (!PyBool_Check(bool)) \
    { \
        PyErr_SetString(PyExc_TypeError, message); \
        return NULL; \
    }
#define PyBool_ConvertChar(bool, chr) chr = (bool == Py_True); \
    if (bool == Py_True) \
        Py_XDECREF(Py_True); \
    else \
        Py_XDECREF(Py_False);
#define PyBool_Decref(bool) if (bool == Py_True) \
    Py_XDECREF(Py_True); \
    else \
        Py_XDECREF(Py_False);
    
        
        
    
typedef struct {
    PyObject_HEAD
    PyObject *object;
    char mark;
} MarkableReference;

static int MarkableReference_init(MarkableReference *self, PyObject *args, 
    PyObject *kwds) 
{
        static char *kwlist[] = {"obj", "mark", NULL};
        PyObject *mark_as_pybool;
        char mark;
        
        if (!__atomic_is_lock_free(sizeof(self->object), &self->object) &&
                !__atomic_is_lock_free(sizeof(self->mark), &self->mark))
        {
            if(PyErr_WarnEx(PyExc_RuntimeWarning, 
                    "atomic.MarkableReference is not lock free", 1) < 0)
                return -1;
        }
        
        self->object = Py_None;
        self->mark = 0;
        mark_as_pybool = Py_False;
        Py_INCREF(Py_False);
        
        if(!PyArg_ParseTupleAndKeywords(args, kwds, "|OO", kwlist, 
                &self->object, &mark_as_pybool))
            return -1; 
        if(!PyBool_Check(mark_as_pybool))
        {
            PyErr_SetString(PyExc_TypeError, "Passed mark is not a boolean");
            return -1;
        }
        PyBool_ConvertChar(mark_as_pybool, mark);
        __atomic_store(&self->mark, &mark, __ATOMIC_SEQ_CST);
            
        Py_INCREF(self->object);
        Py_DECREF(Py_False);
        return 0;
}
    
static int MarkableReference_traverse(MarkableReference *self, 
            visitproc visit, void *arg)
{
    PyObject *object;
        
    __atomic_load(&self->object, &object, __ATOMIC_SEQ_CST);
        
    Py_VISIT(object);
        
    return 0;
}

static int MarkableReference_clear(MarkableReference *self)
{
    PyObject *object;
        
    object = __atomic_exchange_n(&self->object, NULL, __ATOMIC_SEQ_CST);
    __atomic_exchange_n(&self->mark, 0, __ATOMIC_SEQ_CST);
        
    Py_XDECREF(object);
    
    return 0;
}
    
static void MarkableReference_dealloc(MarkableReference *self)
{
    MarkableReference_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}
    
static PyObject *MarkableReference_repr(MarkableReference *self)
{
    PyObject *object;
    char mark;
        
    __atomic_load(&self->object, &object, __ATOMIC_SEQ_CST);
    __atomic_load(&self->mark, &mark, __ATOMIC_SEQ_CST);
    
    return PyUnicode_FromFormat("atomic.MarkableReference(%R, %R)",
        object, mark);
}
    

static PyObject *MarkableReference_get_reference(MarkableReference *self)
{
    PyObject *object;
    
    __atomic_load(&self->object, &object, __ATOMIC_SEQ_CST);
    
    Py_INCREF(object);
    return object;
}

static PyObject *MarkableReference_is_marked(MarkableReference *self)
{
    char mark;
    __atomic_load(&self->mark, &mark, __ATOMIC_SEQ_CST);
    if (mark) 
    {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject *MarkableReference_get(MarkableReference *self)
{
    PyObject *object, *converted_mark;
    char mark;
        
    __atomic_load(&self->object, &object, __ATOMIC_SEQ_CST);
    __atomic_load(&self->mark, &mark, __ATOMIC_SEQ_CST);
    
    if (mark){
        converted_mark = Py_True;
        Py_INCREF(Py_True);
    }
    else{
        converted_mark = Py_False;
        Py_INCREF(Py_False);
    }
    PyObject *output_tuple = PyTuple_New(2);
    if (PyTuple_SetItem(output_tuple, 0, object) != 0) {
        PyErr_SetString(PyExc_ValueError, "Cannot return internal reference" +
        "due to inability to insert in return tuple");
        return NULL;
    }
    if (PyTuple_SetItem(output_tuple, 1, converted_mark) != 0) {
        PyErr_SetString(PyExc_ValueError, "Cannot return mark due to" +
        "inability to insert into return tuple");
        return NULL;
    }
    return output_tuple;
}

static PyObject *MarkableReference_weak_compare_and_set(MarkableReference *self,
        PyObject *args) 
{
    PyObject *expect_obj, *update_obj, *exp_mark_as_pybool, *upd_mark_as_pybool;
    char expect_mark, update_mark;
    long ret;
    
    if(!PyArg_ParseTuple(args, "OOOO", &expect_obj, &update_obj,
        &exp_mark_as_pybool, &upd_mark_as_pybool))
        return NULL;
    
    PyBool_ExcCheck(exp_mark_as_pybool, "Expected mark is not a boolean")
    PyBool_ExcCheck(upd_mark_as_pybool, "Update mark is not a booean")
    PyBool_ConvertChar(exp_mark_as_pybool, expect_mark);
    PyBool_ConvertChar(upd_mark_as_pybool, update_mark);
    Py_INCREF(&update_obj);
    
    ret = __atomic_compare_exchange(&self->object, &expect_obj, &update_obj, 1,
            __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    ret &= __atomic_compare_exchange(&self->mark, &expect_mark, &update_mark, 1,
            __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    
    if (!ret)
        Py_DECREF(update_obj);
    return PyBool_FromLong(ret);
}

static PyObject *MarkableReference_compare_and_set(MarkableReference *self, 
        PyObject *args)
{
    PyObject *expect_obj, *update_obj, *exp_mark_as_pybool, *upd_mark_as_pybool;
    char expect_mark, update_mark;
    long ret;
    
    if(!PyArg_ParseTuple(args, "OOOO", &expect_obj, &update_obj, 
        &exp_mark_as_pybool, &upd_mark_as_pybool))
            return NULL;
    
    Py_INCREF(update_obj);
    
    PyBool_ExcCheck(exp_mark_as_pybool, "Expected mark is not a boolean")
    PyBool_ExcCheck(upd_mark_as_pybool, "Update mark is not a boolean")
    PyBool_ConvertChar(exp_mark_as_pybool, expect_mark);
    PyBool_ConvertChar(upd_mark_as_pybool, update_mark);
    
    ret = __atomic_compare_exchange(&self->object, &expect_obj, &update_obj, 0,
            __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    ret &= __atomic_compare_exchange(&self->mark, &expect_mark, &update_mark, 0,
            __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    
    if(!ret)
        Py_DECREF(update_obj);
    Py_DECREF(expect_obj);
    return PyBool_FromLong(ret);
}
    
static PyObject *MarkableReference_set(MarkableReference *self, PyObject *args)
{
    PyObject *object, *old_object, *mark_as_pybool;
    char mark, old_mark;
    
    if (!PyArg_ParseTuple(args, "OO", &object, &mark_as_pybool))
        return NULL;
    
    PyBool_ExcCheck(mark_as_pybool, "Mark is not a boolean")
    PyBool_ConvertChar(mark_as_pybool, mark);
    Py_INCREF(object);
    
    __atomic_exchange(&self->object, &object, &old_object, __ATOMIC_SEQ_CST);
    __atomic_exchange(&self->mark, &mark, &old_mark, __ATOMIC_SEQ_CST);
    
    Py_DECREF(old_object);
    Py_RETURN_NONE;
}

static PyObject *MarkableReference_attempt_mark(MarkableReference *self, 
    PyObject *args)
{
    PyObject *exp_mark_as_pybool, *upd_mark_as_pybool;
    char expect_mark, update_mark;
    long ret;
    
    if (!PyArg_ParseTuple(args, "OO", &exp_mark_as_pybool, &upd_mark_as_pybool))
        return NULL;
    
    PyBool_ExcCheck(exp_mark_as_pybool, "Expected mark is not a boolean")
    PyBool_ExcCheck(upd_mark_as_pybool, "Update mark is not a boolean")
    PyBool_ConvertChar(exp_mark_as_pybool, expect_mark);
    PyBool_ConvertChar(upd_mark_as_pybool, update_mark);
    
    ret = __atomic_compare_exchange(&self->mark, &expect_mark, &update_mark, 0,
            __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    
    return PyBool_FromLong(ret);
}

static PyMethodDef MarkableReference_methods[] = {
   {"get_reference", (PyCFunction)MarkableReference_get_reference, METH_NOARGS,
    "get_reference() -> object\n\n"
    "Atomically load and return the stored reference."},
   {"is_marked", (PyCFunction)MarkableReference_is_marked, METH_NOARGS,
    "is_marked() -> bool\n\n"
    "Atomically loads and returns the given mark."},
   {"get", (PyCFunction)MarkableReference_get, METH_NOARGS,
    "get() -> (object, mark) \n\n"
    "Returns the current values of both the reference and the mark. "
   },
   {"weak_compare_and_set", (PyCFunction)MarkableReference_weak_compare_and_set,
    METH_VARARGS, "weak_compare_and_set(expect_ref, update_ref, expect_mark,"
    "update_mark) -> bool\n\nAtomically stores the given mark and reference"
    "if the old mark and reference equals the given expected mark and reference"
    "by identitym returning whether the actual reference equaled the expected"
    "value. Can fail spuriously and does not provide ordering guarantees."},
   {"compare_and_set", (PyCFunction)MarkableReference_compare_and_set, 
   METH_VARARGS, "compare_and_set(expect_ref, update_ref, expect_mark,"
   "update_mark) -> bool\n\nAtomically stores the given mark and reference if"
   "the old mark and reference equals the expected mark and reference by"
   "identity, returning whether or not the actual mark and reference equaled"
   "the expected values."},
   {"set", (PyCFunction)MarkableReference_set, METH_VARARGS, "set(obj, mark)"
    "\n\nAtomically stores the given mark and reference."},
   {"attempt_mark", (PyCFunction)MarkableReference_attempt_mark, METH_VARARGS,
    "attempt_mark(expect_mark, update_mark) -> bool\n\nAtomically sets the"
    "value of the mark to the given update value if the current reference is"
    "equal to the expected reference by identity. Any given invocation of this"
    "operation may fail (return false) spuriously, but repeated invocation when"
    "the current value holds the expected value and no other thread is also"
    "attempting to set the value will eventually succeed."},
    {NULL, NULL, 0, NULL}
};

#define ATOMIC_MARKABLE_REFERENCE_DOCSTRING \
        "atomic.MarkableReference(obj = None, mark = False) -> new atomic" \
        "markable reference \n\nMarkable reference supporting atomic" \
        "operations with sequentially consistent semantics.\n\n Atomic load" \
        "(get()), store (set()), compare-and-exchange(compare_and_set())," \
        "mark (attempt_mark()) are supported."
        
PyTypeObject MarkableReference_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "atomic.MarkableReference",  /* tp_name */
    sizeof(MarkableReference),   /* tp_basicsize */
    0,      /*tp_itemsize */
    (destructor)MarkableReference_dealloc, /* tp_dealloc */ 
    NULL,                       /* tp_print */
    NULL,					/* tp_getattr */
	NULL,					/* tp_setattr */
	NULL,					/* tp_reserved */
	(reprfunc)MarkableReference_repr,    /*tp_repr*/
	NULL,					/* tp_as_number */
	NULL,					/* tp_as_sequence */
	NULL,					/* tp_as_mapping */
	NULL,					/* tp_hash  */
	NULL,					/* tp_call */
	NULL,					/* tp_str */
	NULL,					/* tp_getattro */
	NULL,					/* tp_setattro */
	NULL,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,     /* tp_flags */
	ATOMIC_MARKABLE_REFERENCE_DOCSTRING, /* tp_doc */
	(traverseproc)MarkableReference_traverse,   /* tp_traverse */
	(inquiry)MarkableReference_clear,       /* tp_clear */
	NULL,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	NULL,					/* tp_iter */
	NULL,					/* tp_iternext */
	MarkableReference_methods,			/* tp_methods */
	NULL,					/* tp_members */
	NULL,					/* tp_getset */
	NULL,					/* tp_base */
	NULL,					/* tp_dict */
	NULL,					/* tp_descr_get */
	NULL,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	(initproc)MarkableReference_init,		/* tp_init */
};

