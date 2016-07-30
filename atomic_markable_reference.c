#include <Python.h>

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
        
        if(!PyArg_ParseTupleAndKeywords(args, kwds, "|OO", kwlist, 
                &self->object, &mark_as_pybool))
            return -1;
        if(!PyBool_Check(mark_as_pybool))
            return -1;
        if (mark_as_pybool == Py_True)
            mark = 1;
        else 
            mark = 0;
        __atomic_load(&self->mark, &mark, __ATOMIC_SEQ_CST);
            
        Py_INCREF(self->object);
        
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
    if (0x01 & self->mark) 
    {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject *MarkableReference_get(MarkableReference *self, 
    PyObject *args)
{
    PyObject *markholder;
    if (!PyArg_ParseTuple(args, "O",&markholder))
        return NULL;
    
    PyObject *object, *converted_mark;
    char mark;
        
    __atomic_load(&self->object, &object, __ATOMIC_SEQ_CST);
    __atomic_load(&self->mark, &mark, __ATOMIC_SEQ_CST);
    
    if (mark & 0x01){
        converted_mark = Py_True;
        Py_INCREF(Py_True);
    }
    else{
        converted_mark = Py_False;
        Py_INCREF(Py_False);
    }
    
    if(!PyList_Size(markholder)) {
        PyList_Append(markholder, converted_mark);
        //No DECREF due to char
    }
    else
        PyList_SetItem(markholder, 0, converted_mark);
        
    //Do not need to increase reference count for markholder since it was
    //created externally to the _get function
    Py_INCREF(object);
    return object;
}

static PyObject *MarkableReference_weak_compare_and_set(MarkableReference *self,
        PyObject *args) 
{
    PyObject *expect_obj, *update_obj;
    char expect_mark, update_mark;
    long ret;
    
    if(!PyArg_ParseTuple(args, "OOcc", &expect_obj, &update_obj,
        &expect_mark, &update_mark))
        Py_RETURN_FALSE;
    
    Py_INCREF(&update_obj);
    
    ret = __atomic_compare_exchange(&self->object, &expect_obj, &update_obj, 1,
            __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    ret &= __atomic_compare_exchange(&self->mark, &expect_mark, &update_mark, 1,
            __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    
    if (!ret)
        Py_DECREF(update_obj);
    Py_DECREF(expect_obj);
    return PyBool_FromLong(ret);
}

static PyObject *MarkableReference_compare_and_set(MarkableReference *self, 
        PyObject *args)
{
    PyObject *expect_obj, *update_obj;
    char expect_mark, update_mark;
    long ret;
    
    if(!PyArg_ParseTuple(args, "OOcc", &expect_obj, &update_obj, &expect_mark,
            &update_mark))
        Py_RETURN_FALSE;
    
    Py_INCREF(update_obj);
    
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
    PyObject *object, *old_object;
    char mark, old_mark;
    
    if (!PyArg_ParseTuple(args, "Oc", &object, &mark))
        return NULL;
    
    Py_INCREF(object);
    
    __atomic_exchange(&self->object, &object, &old_object, __ATOMIC_SEQ_CST);
    __atomic_exchange(&self->mark, &mark, &old_mark, __ATOMIC_SEQ_CST);
    
    Py_DECREF(old_object);
    Py_RETURN_NONE;
}

static PyObject *MarkableReference_attempt_mark(MarkableReference *self, 
    PyObject *args)
{
    
    char expect_mark, update_mark;
    long ret;
    
    if (!PyArg_ParseTuple(args, "cc", &expect_mark, &update_mark))
        Py_RETURN_FALSE;
    
    ret = __atomic_compare_exchange(&self->mark, &expect_mark, &update_mark, 0,
            __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    
    return PyBool_FromLong(ret);
    
}

static PyMethodDef MarkableReference_methods[] = {
   {"getReference", (PyCFunction)MarkableReference_get_reference, METH_NOARGS,
    "getReference() -> object\n\n"
    "Atomically load and return the stored reference."},
   {"is_marked", (PyCFunction)MarkableReference_is_marked, METH_NOARGS,
    "isMarked() -> bool\n\n"
    "Atomically loads and returns the given mark."},
   {"get", (PyCFunction)MarkableReference_get, METH_NOARGS,
    "get(markholder) -> object \n\n"
    "Returns the current values of both the reference and the mark. Typical"
    "usage is a bool list. reference = markable.get(markholder)."
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

