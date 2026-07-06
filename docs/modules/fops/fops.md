# FOPS

FOPS is the File Operations Layer for MirageFS.

It accepts already resolved `fuid_t` values plus single path components and performs one filesystem operation through Object and LSA. It does not parse filesystem names, full paths, cwd, or server requests.

Stable identity is based on Object Layer handle indexing:

```text
Linux backend identity: mount_id + file_handle
MirageFS runtime identity: objectid + gen
```

When FOPS discovers an object, it first looks up the backend handle in ObjMgr. Existing objects reuse their objectid/gen. New discoveries allocate an objectid through ObjMgr and register the handle mapping.
