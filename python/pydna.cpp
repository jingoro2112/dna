/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "pydna.h"
#include "../util/read_hex.hpp"
#include "../util/str.hpp"
#include "../util/simple_log.hpp"

#include "../splice/splice.hpp"

SimpleLog Log;


PYDNA_API PyDNA* pyDNA = 0;
const char* g_moduleName = "dna";
DNADEVICE PyDNA::m_device = INVALID_DNADEVICE_VALUE;

//------------------------------------------------------------------------------
PyMethodDef PyDNA::m_methods[] =
{
	{ "about", PyAbout, METH_VARARGS, "about()\ndescribes current version\n" },

	{ "open", PyOpen, METH_VARARGS, "open()\nattempt to open the DNA device" },
	{ "close", PyClose, METH_VARARGS, "close()\nclose down an open DNA device" },
	{ "flashClient", PyFlashClient, METH_VARARGS, "flashClient()\nflash the Python embedded client onto the DNA device" }, 
	
	{ 0, 0, 0, 0 },
};

static const char* s_DNADocstring = "DNA Python interface Version 0.01";

//------------------------------------------------------------------------------
PyMODINIT_FUNC
#if PY_MAJOR_VERSION >= 3
PyInit_dna()
{
	return PyDNA::Init();
}
#else
initdna()
{
	PyDNA::Init();
}
#endif

//------------------------------------------------------------------------------
PyObject* PyDNA::Init()
{
	PyEval_InitThreads();

	pyDNA = new PyDNA;

#if PY_MAJOR_VERSION >= 3

	static struct PyModuleDef moduledef =
	{
		PyModuleDef_HEAD_INIT,
		g_moduleName,
		s_DNADocstring,
		-1,                  // m_size
		PyDNA::m_methods,    // m_methods
		NULL,                // m_reload
		NULL,                // m_traverse 
		NULL,                // m_clear
		NULL,                // m_free
	};

	return PyModule_Create( &moduledef );
#else
	return Py_InitModule( g_moduleName, PyDNA::m_methods );
#endif
}

//------------------------------------------------------------------------------
PyObject* PyDNA::PyAbout( PyObject *self, PyObject *args )
{
#if PY_MAJOR_VERSION >= 3
	return PyBytes_FromString( s_DNADocstring );
#else
	return PyString_FromString( s_DNADocstring );
#endif
}

//------------------------------------------------------------------------------
PyObject* PyDNA::PyOpen( PyObject *self, PyObject *args )
{
	m_device = DNAUSB::openDevice( 0x16C0, 0x05DF, "p@northarc.com" );

	if( m_device != INVALID_DNADEVICE_VALUE )
	{
		Py_RETURN_TRUE;
	}
	else
	{
		Py_RETURN_FALSE;
	}
}

//------------------------------------------------------------------------------
PyObject* PyDNA::PyClose( PyObject *self, PyObject *args )
{
	DNAUSB::closeDevice( m_device );
	m_device = INVALID_DNADEVICE_VALUE;
	Py_RETURN_NONE;
}

//------------------------------------------------------------------------------
PyObject* PyDNA::PyFlashClient( PyObject *self, PyObject *args )
{
	DNAUSB::closeDevice( m_device );
	m_device = INVALID_DNADEVICE_VALUE;

	CLinkList<ReadHex::Chunk> chunklist;

	Cstr infile;
	if ( !infile.fileToBuffer("python_client.hex") )
	{
		Py_RETURN_FALSE;
	}

	if ( !ReadHex::parse(chunklist, infile.c_str(), infile.size()) )
	{
		Py_RETURN_FALSE;
	}

	if ( chunklist.count() != 1 )
	{
		Py_RETURN_FALSE;
	}

	ReadHex::Chunk *chunk = chunklist.getFirst();
	if ( !Splice::checkDNAImage(chunk->data, chunk->size) )
	{
		Py_RETURN_FALSE;
	}

	if ( DNAUSB::sendCode(0x16C0, 0x05DF, "p@northarc.com", (unsigned char *)(chunk->data), chunk->size, 0) )
	{
		Py_RETURN_TRUE;
	}
	else
	{
		Py_RETURN_FALSE;
	}
}
