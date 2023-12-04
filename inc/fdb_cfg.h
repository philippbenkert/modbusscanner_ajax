#ifndef _FDB_CFG_H_
#define _FDB_CFG_H_

#define FDB_USING_KVDB
//#define FDB_USING_TSDB  // Deaktiviert, wenn nicht benötigt

//#define FDB_USING_FAL_MODE  // Deaktivieren, wenn FAL nicht verwendet wird
#define FDB_USING_FILE_LIBC_MODE  // Aktivieren für Standard-Dateioperationen

#define FDB_WRITE_GRAN 1  // Setzen Sie dies auf die Schreibgranularität Ihres Speichers

#define FDB_DEBUG_ENABLE  // Aktiv für Debugging, deaktivieren für Produktion

#endif /* _FDB_CFG_H_ */
