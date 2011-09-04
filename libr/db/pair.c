/* radare - LGPL - Copyright 2011 pancake<nopcode.org> */

#include <r_db.h>
#include <r_util.h>
//#undef UT32_MAX
//#undef UT64_MAX
#include "sdb/src/sdb.h"

/*
var p = new Pair ();
p.set ("foo", "bar")
var out = p.get ("foo")
*/

R_API RPair *r_pair_new () {
	RPair *p = R_NEW0 (RPair);
	p->ht = r_hashtable_new ();
	p->dbs = r_list_new ();
	p->dbs->free = (RListFree)sdb_free;
	return p;
}

R_API void r_pair_free (RPair *p) {
	r_hashtable_free (p->ht);
	r_list_destroy (p->dbs);
	free (p->dir);
	free (p);
}

R_API void r_pair_delete (RPair *p, const char *name) {
	Sdb *sdb;
	ut32 hdom;
	char *dom, *key = strdup (name);

	dom = r_str_lchr (key, '.');
	if (dom) {
		key = dom+1;
		*dom = 0;
		dom = 0;
	} else dom = "";
	hdom = r_str_hash (dom);
	sdb = r_hashtable_lookup (p->ht, hdom);
	if (sdb)
		sdb_delete (sdb, key);
}

static Sdb *pair_sdb_new(RPair *p, const char *dom, ut32 hdom) {
	Sdb *sdb;
	char *old = NULL;
	if (p->dir) {
		old = r_sys_getdir ();
		r_sys_rmkdir (p->dir);
		r_sys_chdir (p->dir);
	}

	sdb = sdb_new (dom, 0);
	if (old) {
		r_sys_chdir (old);
		free (old);
	}
	r_list_append (p->dbs, sdb);
	r_hashtable_insert (p->ht, hdom, sdb);
	return sdb;
}

R_API char *r_pair_get (RPair *p, const char *name) {
	Sdb *sdb;
	ut32 hdom;
	char *dom, *key, *okey = strdup (name);

	key = okey;
	dom = r_str_lchr (okey, '.');
	if (dom) {
		char *tmp = okey;
		*dom = 0;
		key = dom+1;
		dom = tmp;
	} else dom = "";
	hdom = r_str_hash (dom);
	sdb = r_hashtable_lookup (p->ht, hdom);
	if (!sdb)
		sdb = pair_sdb_new (p, dom, hdom);
	dom = sdb_get (sdb, key);
	free (okey);
	return dom;
}

R_API void r_pair_set (RPair *p, const char *name, const char *value) {
	Sdb *sdb;
	ut32 hdom;
	char *dom, *key = strdup (name);

	dom = r_str_lchr (key, '.');
	if (dom) {
		char *okey = key;
		*dom = 0;
		key = dom+1;
		dom = okey;
	} else dom = "";
	hdom = r_str_hash (dom);
	sdb = r_hashtable_lookup (p->ht, hdom);
	if (!sdb)
		sdb = pair_sdb_new (p, dom, hdom);
	sdb_set (sdb, key, value);
}

R_API RList *r_pair_list (RPair *p, const char *domain) {
	ut32 hdom = r_str_hash (domain);
	Sdb *s = r_hashtable_lookup (p->ht, hdom);
	if (s) {
		RList *list = r_list_new ();
		char key[SDB_KEYSIZE];
		char val[SDB_VALUESIZE];
		list->free = (RListFree)r_pair_item_free;
		sdb_dump_begin (s);
		while (sdb_dump_next (s, key, val))
			r_list_append (list, r_pair_item_new (key, val));
		return list;
	}
	return NULL;
}

RPairItem *r_pair_item_new (const char *k, const char *v) {
	RPairItem *i = R_NEW (RPairItem);
	i->k = strdup (k);
	i->v = strdup (v);
	return i;
}

R_API void r_pair_item_free (RPairItem *i) {
	free (i->k);
	free (i->v);
	free (i);
}

R_API void r_pair_set_sync_dir (RPair *p, const char *dir) {
	free (p->dir);
	p->dir = strdup (dir);
}

// use sync dir
//R_API void r_pair_load (RPair *p) { /* TODO */ }

R_API void r_pair_reset (RPair *p) {
	Sdb *s;
	RListIter *iter;
	r_list_foreach (p->dbs, iter, s)
		sdb_reset (s);
}

R_API void r_pair_sync (RPair *p) {
	Sdb *s;
	char *old = NULL;
	RListIter *iter;
	if (p->dir) {
		old = r_sys_getdir ();
		r_sys_rmkdir (p->dir);
		r_sys_chdir (p->dir);
	}
	r_list_foreach (p->dbs, iter, s) {
		sdb_sync (s);
	}
	if (old) {
		r_sys_chdir (old);
		free (old);
	}
}