/* mtest6.c - memory-mapped database tester/toy */
/*
 * Copyright 2011-2021 Howard Chu, Symas Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

/* Tests for DB splits and merges */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lmdb.h"

#define E(expr) CHECK((rc = (expr)) == MDB_SUCCESS, #expr)
#define RES(err, expr) ((rc = expr) == (err) || (CHECK(!rc, #expr), 0))
#define CHECK(test, msg) ((test) ? (void)0 : ((void)fprintf(stderr, \
	"%s:%d: %s: %s\n", __FILE__, __LINE__, msg, mdb_strerror(rc)), abort()))

char dkbuf[1024];

#define DKBUF_MAXKEYSIZE (511)

/** Display a key in hexadecimal and return the address of the result.
 * @param[in] key the key to display
 * @param[in] buf the buffer to write into. Should always be #DKBUF.
 * @return The key in hexadecimal form.
 */
char *
mdb_dkey(MDB_val *key, char *buf)
{
	if (!key)
		return "";

	char *ptr = buf;
	unsigned char *c = key->mv_data;
	unsigned int i;

	if (key->mv_size > DKBUF_MAXKEYSIZE)
		return "MDB_MAXKEYSIZE";

	buf[0] = '\0';
	for (i=0; i<key->mv_size; i++)
		ptr += sprintf(ptr, "%02x", *c++);

	return buf;
}

int main(int argc,char * argv[])
{
	int i = 0, rc;
	MDB_env *env;
	MDB_dbi dbi;
	MDB_val key, data, sdata;
	MDB_txn *txn;
	MDB_stat mst;
	MDB_cursor *cursor;
	unsigned long kval;
	char *sval;

	srand(time(NULL));

	E(mdb_env_create(&env));
	E(mdb_env_set_mapsize(env, 10485760));
	E(mdb_env_set_maxdbs(env, 4));
	E(mdb_env_open(env, "./testdb", MDB_FIXEDMAP|MDB_NOSYNC, 0664));

	E(mdb_txn_begin(env, NULL, 0, &txn));
	E(mdb_dbi_open(txn, "id6", MDB_CREATE|MDB_INTEGERKEY, &dbi));
	E(mdb_cursor_open(txn, dbi, &cursor));
	E(mdb_stat(txn, dbi, &mst));

	sval = calloc(1, mst.ms_psize / 4);
	key.mv_size = sizeof(long);
	key.mv_data = &kval;
	sdata.mv_size = mst.ms_psize / 4 - 30;
	sdata.mv_data = sval;

	printf("Adding 12 values, should yield 3 splits\n");
	for (i=0;i<12;i++) {
		kval = i*5;
		sprintf(sval, "%08lx", kval);
		data = sdata;
		(void)RES(MDB_KEYEXIST, mdb_cursor_put(cursor, &key, &data, MDB_NOOVERWRITE));
	}
	printf("Adding 12 more values, should yield 3 splits\n");
	for (i=0;i<12;i++) {
		kval = i*5+4;
		sprintf(sval, "%08lx", kval);
		data = sdata;
		(void)RES(MDB_KEYEXIST, mdb_cursor_put(cursor, &key, &data, MDB_NOOVERWRITE));
	}
	printf("Adding 12 more values, should yield 3 splits\n");
	for (i=0;i<12;i++) {
		kval = i*5+1;
		sprintf(sval, "%08lx", kval);
		data = sdata;
		(void)RES(MDB_KEYEXIST, mdb_cursor_put(cursor, &key, &data, MDB_NOOVERWRITE));
	}
	E(mdb_cursor_get(cursor, &key, &data, MDB_FIRST));

	do {
		printf("key: %p %s, data: %p %.*s\n",
			key.mv_data,  mdb_dkey(&key, dkbuf),
			data.mv_data, (int) data.mv_size, (char *) data.mv_data);
	} while ((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0);
	CHECK(rc == MDB_NOTFOUND, "mdb_cursor_get");
	mdb_cursor_close(cursor);
	mdb_txn_commit(txn);
	mdb_env_close(env);

	return 0;
}
