API Reference
=============

ts_status
---------

.. code:: c

   typedef enum ts_status {
     TS_OK = 0,
     TS_FOPEN_ERR = -1,
     TS_FCLOSE_ERR = -2,
     TS_TYPE_ERR = -3,
   } ts_status;

This enum contains error codes that any function can return.

ts_load_file
------------

.. code:: c

   ts_status ts_load_file(ts_interp *sc, const char *name);

Loads and runs the given file. If there are no errors it will return
``TS_OK`` and in case of errors ``TS_FOPEN_ERR`` or ``TS_FCLOSE_ERR``.

ts_vec_len
----------

.. code:: c

   int ts_vec_len(ts_ptr vec);

Gets the vector length, in case of error can return ``TS_TYPE_ERR``.

ts_get_global
-------------

.. code:: c

   ts_ptr ts_get_global(ts_interp *sc, ts_ptr env, const char *name);

Gets a scheme value from environment ``env`` by name, in case of error return ``sc->nil``.

ts_is_userdata
--------------

.. code:: c

   bool ts_is_userdata(ts_ptr ptr);

Checks if the given scheme pointer is ``userdata``.

ts_userdata_set_finalizer
-------------------------

.. code:: c

   void ts_userdata_set_finalizer(ts_ptr userdata, void (*finalizer)(void *))

Sets the finalizer function (destructor) for the userdata value.

ts_mk_userdata
--------------

.. code:: c

   ts_ptr ts_mk_userdata(ts_interp *sc, void *ptr);

Creates a scheme userdata from any C pointer.

ts_mk_bool
----------

.. code:: c

   ts_ptr ts_mk_bool(ts_interp *sc, bool b);

Converts C bool to scheme bool.
