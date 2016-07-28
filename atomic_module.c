#include <Python.h>

#define ATOMIC_MODULE_NAME "atomic"
#define ATOMIC_MODULE_DOCSTRING \
	"Module providing types supporting atomic operations."

extern PyTypeObject Integer_type, Reference_type, MarkableReference_type;

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef atomicmodule = {
	PyModuleDef_HEAD_INIT,
	ATOMIC_MODULE_NAME,
	ATOMIC_MODULE_DOCSTRING,
	-1,
};

#define INITERROR return NULL;

PyMODINIT_FUNC PyInit_atomic(void)
#else
#define INITERROR return;

void initatomic(void)
#endif
{
	PyObject *m;

	Integer_type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&Integer_type) < 0)
		INITERROR;

	Reference_type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&Reference_type) < 0)
		INITERROR;
		
	MarkableReference.tp_new = PyType_GenericNew;
	if(PyType_Ready(&MarkableReference) < 0)
		INITERROR;

#if PY_MAJOR_VERSION >= 3
	m = PyModule_Create(&atomicmodule);
#else
	m = Py_InitModule3(ATOMIC_MODULE_NAME, NULL, ATOMIC_MODULE_DOCSTRING);
#endif
	if (m == NULL)
		INITERROR;

	Py_INCREF(&Integer_type);
	PyModule_AddObject(m, "Integer", (PyObject *)&Integer_type);

	Py_INCREF(&Reference_type);
	PyModule_AddObject(m, "Reference", (PyObject *)&Reference_type);
	
	Py_INCREF(&MarkableReference_Type);
	PyModule_AddObject(m, "MarkableReference", (PyObject *)&MarkableReference);

#if PY_MAJOR_VERSION >= 3
	return m;
#endif
}
