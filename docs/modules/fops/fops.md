# FOPS

FOPS is the File Operations Layer for MirageFS.

It accepts already resolved `fuid_t` values plus single path components and performs one filesystem operation through Object and LSA. It does not parse full paths, cwd, mount names, or server requests.

## Position

```text
upper layer / future NAMEI
        |
        v
FOPS: one parent FUID + one component + one operation
        |
        +--> ObjMgr: runtime identity, handle index, objectid/gen allocation
        |
        +--> LSA: Linux syscall boundary
```

Stable backend identity is `mount_id + file_handle`. MirageFS runtime identity is `objectid + gen` inside `fuid_t`.

When FOPS discovers an object, it first looks up the backend handle in ObjMgr. Existing objects reuse their current `objectid/gen`. New discoveries allocate an `obj_key_t` through ObjMgr and register the backend handle mapping.

## Object Identity

`objectid` is allocated by MirageFS, not copied from Linux inode/stat fields. ObjMgr owns the allocator and returns an `obj_key_t`:

```text
obj_key_t = objectid slot + generation
fuid_t    = fsid + objectid + gen + type + view fields
```

When an object runtime is reclaimed, ObjMgr frees the slot and advances `gen`. A stale FUID with an old generation will no longer match the new runtime object after slot reuse.

## Name Rules

All FOPS APIs accept a single path component only. Slash-separated paths are rejected.

Lookup accepts `.` and `..`:

- `.` resolves to the parent FUID itself and reads its attributes.
- `..` is delegated to LSA/Linux lookup semantics for the current backend directory.

Create/delete APIs reject `.` and `..`.

## Flags

Common flags are defined in `src/common/flag/fs_flag.h`.

Recommended semantics:

| Flag | Meaning |
| --- | --- |
| `FS_FLAG_NONE` | No extra constraint. |
| `FS_FLAG_REPLACE` | Creation may reuse an existing regular file. Does not imply truncate. |
| `FS_FLAG_EXCLUSIVE` | Target must not already exist. Conflicts with `REPLACE`. |
| `FS_FLAG_NOFOLLOW` | Do not follow final symlink component where the backend operation supports it. |
| `FS_FLAG_SYNC` | Open created regular files with synchronous write semantics. |
| `FS_FLAG_DIRECT` | Request direct I/O for created regular files when supported by the platform. |
| `FS_FLAG_DIRECTORY` | Result/target must be a directory. |
| `FS_FLAG_REGULAR` | Result/target must be a regular file. |
| `FS_FLAG_TRUNCATE` | Truncate a created/reused regular file. Separate from `REPLACE`. |
| `FS_FLAG_APPEND` | Open created/reused regular files in append mode. |

Operation support:

| Operation | Supported flags |
| --- | --- |
| `lookup`, `lookup_plus` | `NOFOLLOW`, `DIRECTORY`, `REGULAR` |
| `create`, `create_plus` | `REPLACE`, `EXCLUSIVE`, `NOFOLLOW`, `SYNC`, `DIRECT`, `REGULAR`, `TRUNCATE`, `APPEND` |
| `mkdir`, `mkdir_plus` | `EXCLUSIVE`, `DIRECTORY` |
| `getattr` | `DIRECTORY`, `REGULAR` |
| `readdir`, `readdirplus` | `DIRECTORY` |
| `unlink` | `NOFOLLOW`, `REGULAR` |
| `rmdir` | `DIRECTORY` |

Unknown or unsupported flag bits are rejected with `EINVAL`.

## Attributes

`fops_attr_t` is an output snapshot. It is returned by `getattr`, `lookup_plus`, `create_plus`, `mkdir_plus`, and `readdirplus`.

`fops_create_attr_t` is a create-time request. `valid_mask` controls which fields are applied:

| Mask | Applies to | Meaning |
| --- | --- | --- |
| `FOPS_CREATE_ATTR_MODE` | `create`, `mkdir` | Permission bits for the new object. |
| `FOPS_CREATE_ATTR_UID` | `create`, `mkdir` | Owner uid. |
| `FOPS_CREATE_ATTR_GID` | `create`, `mkdir` | Owner gid. |
| `FOPS_CREATE_ATTR_SIZE` | `create` only | Initial regular-file size via truncate. |

If `attr == NULL`, defaults are used: regular files use `FS_MODE_FILE_DEFAULT`, directories use `FS_MODE_DIR_DEFAULT`.

## Plus APIs

Lightweight APIs return only `fuid_t`:

```c
fops_lookup(..., fuid_t *out_fuid);
fops_create(..., fuid_t *out_fuid);
fops_mkdir(..., fuid_t *out_fuid);
fops_readdir(..., fops_dirent_t *entries, ...);
```

Plus APIs return `fuid_t` and attributes in the same operation:

```c
fops_lookup_plus(..., fops_object_result_t *out);
fops_create_plus(..., fops_object_result_t *out);
fops_mkdir_plus(..., fops_object_result_t *out);
fops_readdirplus(..., fops_dirent_plus_t *entries, ...);
```

Current implementation uses the plus path as the core for lookup/create/mkdir, and the lightweight wrappers copy only the returned FUID.

`readdir` and `readdirplus` share an internal iterator helper. `readdir` avoids stat when the backend provides a usable directory-entry type, and falls back to `fstatat` only for unknown types. `readdirplus` always returns stat-derived attributes.

## FSC Boundary Note

FSC registers each filesystem root as a normal ObjMgr object so FOPS can operate uniformly on root and non-root directories. FSC still owns namespace lifecycle and `fsid`; ObjMgr only owns runtime object identity and backend handle lookup.

## Completed OP Coverage

FOPS currently exposes one-step APIs for the operation words in `common/op`:

| OP | FOPS API |
| --- | --- |
| `LOOKUP` | `fops_lookup`, `fops_lookup_plus` |
| `CREATE` | `fops_create`, `fops_create_plus` |
| `MKDIR` | `fops_mkdir`, `fops_mkdir_plus` |
| `MKNOD` | `fops_mknod`, `fops_mknod_plus` |
| `UNLINK` | `fops_unlink` |
| `RMDIR` | `fops_rmdir` |
| `RENAME` | `fops_rename` |
| `LINK` | `fops_link`, `fops_link_plus` |
| `SYMLINK` | `fops_symlink`, `fops_symlink_plus` |
| `OPEN` / `OPENHANDLE` | `fops_open`, `fops_openhandle` |
| `CLOSE` | `fops_close` |
| `GETHANDLE` | `fops_gethandle` |
| `GETATTR` / `SETATTR` | `fops_getattr`, `fops_setattr` |
| `ACCESS` | `fops_access` |
| `READ` / `WRITE` | `fops_read`, `fops_write`, `fops_pread`, `fops_pwrite` |
| `TRUNCATE` | `fops_truncate` |
| `GETXATTR` / `SETXATTR` | `fops_getxattr`, `fops_setxattr` |
| `LISTXATTR` / `REMOVEXATTR` | `fops_listxattr`, `fops_removexattr` |
| `READDIR` / `READDIRPLUS` | `fops_readdir`, `fops_readdirplus` |
| `STATFS` / `SYNCFS` | `fops_statfs`, `fops_syncfs` |

`fops_open()` and `fops_openhandle()` return an opaque `fops_file_t`. Callers must release it with `fops_close()`. FOPS does not expose Linux fd ownership to upper layers.

Create-like name operations have conservative replacement semantics:

- `create` supports `REPLACE` because Linux open can atomically reuse an existing regular file.
- `rename` supports `REPLACE` and removes the overwritten target from ObjMgr after success.
- `link`, `symlink`, and `mknod` do not support `REPLACE`; callers get `EEXIST` if the target name already exists.

`mknod` currently supports `MODE` from `fops_create_attr_t`. UID/GID/SIZE are rejected for mknod because applying them safely to FIFO/device nodes needs additional LSA path/fd semantics.
