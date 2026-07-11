# FOPS 妯″潡鎬荤翰

FOPS锛團ile Operations锛夋槸 MirageFS 鐨勫崟姝ユ枃浠舵搷浣滃眰銆傚畠鍚戜笂鎻愪緵缁熶竴鐨勬枃浠舵搷浣滃叆鍙ｏ紝鍚戜笅缁勫悎 ObjMgr銆丗SC 涓?LSA 鑳藉姏锛岃礋璐ｆ妸涓婂眰 OP 璇锋眰杞崲涓烘竻鏅般€佸彲鏍￠獙銆佸彲杩借釜鐨勬枃浠剁郴缁熻涔夈€?
鏈枃妗ｅ彧淇濈暀鎬荤翰涓庡鑸紱姣忕被 OP 鐨勫弬鏁般€乫lag銆佽繑鍥炶涔夊拰瀹炵幇绾︽潫鏀惧湪鍚岀洰褰曠殑涓撻鏂囨。涓€?
## 妯″潡瀹氫綅

FOPS 鐨勬牳蹇冭亴璐ｏ細

- 鎻愪緵缁熶竴鍏ュ彛 `fops_dispatch(args)`锛屼篃淇濈暀缁嗙矑搴?C API 渚夸簬鍐呴儴澶嶇敤鍜屽崟娴嬭鐩栥€?- 浣跨敤 `fops_args_t` 鎵胯浇鎵€鏈?OP 鍙傛暟锛屽叕鍏卞瓧娈垫斁鍦ㄥ灞傦紝宸紓瀛楁鏀惧湪 union 涓€?- 浣跨敤 `fops_op_spec_t` 鎻忚堪姣忎釜 OP 鐨勫弬鏁拌鍒欍€乫lag 鐧藉悕鍗曘€乫lag 鍐茬獊鍜屽熀纭€鏍￠獙瑙勫垯銆?- 浠?`obj_fuid_t` 浣滀负瀵硅薄韬唤锛宱bjectid/gen 鐢?MirageFS 鐨勫璞′綋绯诲垎閰嶅拰绠＄悊銆?- 瀵圭洰褰曢」銆佸睘鎬с€佸彞鏌勩€佽鍐欍€亁attr銆乫s 绾ф搷浣滄彁渚涘崟姝ヨ涔夊皝瑁呫€?
## 鍒嗗眰鍏崇郴

```text
涓婂眰璋冪敤鑰?  鈫?FOPS dispatch / spec / validate
  鈫?FOPS 缁嗙矑搴?OP
  鈫?FSC / ObjMgr / LSA
  鈫?Linux / 鍚庣鏂囦欢绯荤粺
```

FOPS 涓嶇洿鎺ユ壙鎷呭叏灞€鏂囦欢绯荤粺娉ㄥ唽鑱岃矗锛涙枃浠剁郴缁熷疄渚嬪拰鍛藉悕绌洪棿浠嶇敱 FSC 绠＄悊銆侳OPS 鍙湪鎵ц OP 鏃舵秷璐?FSC 鎻愪緵鐨勪笂涓嬫枃銆?
## 鏂囨。鐩綍

- [dispatch.md](dispatch.md)锛氱粺涓€鍏ュ彛銆佸弬鏁扮粨鏋勩€丱P 琛ㄥ拰鏍￠獙娴佺▼銆?- [identity.md](identity.md)锛欶UID銆乷bjectid銆乬en 涓庡璞＄敓鍛藉懆鏈熴€?- [lookup.md](lookup.md)锛歭ookup / lookup_plus銆?- [create.md](create.md)锛歝reate / create_plus銆?- [mkdir.md](mkdir.md)锛歮kdir / mkdir_plus銆?- [mknod.md](mknod.md)锛歮knod / mknod_plus銆?- [unlink.md](unlink.md)锛歶nlink銆?- [rmdir.md](rmdir.md)锛歳mdir銆?- [rename.md](rename.md)锛歳ename銆?- [link.md](link.md)锛歭ink / link_plus / symlink / symlink_plus銆?- [attr.md](attr.md)锛歡etattr / setattr / access / truncate銆?- [readdir.md](readdir.md)锛歳eaddir / readdirplus銆?- [handle.md](handle.md)锛歰pen / openhandle / close / gethandle銆?- [rw.md](rw.md)锛歳ead / write / pread / pwrite銆?- [xattr.md](xattr.md)锛氭墿灞曞睘鎬ф搷浣溿€?- [fs.md](fs.md)锛歴tatfs / syncfs銆?- [flags.md](flags.md)锛歠lag 鎬昏〃鍜屽悇 OP 鏀寔鐭╅樀銆?- [errors.md](errors.md)锛氶敊璇爜鏄犲皠涓庤繑鍥炵害瀹氥€?
## 婧愮爜瀵瑰簲鍏崇郴

```text
src/fops/
  core/      缁熶竴鍏ュ彛銆丱P spec銆佸弬鏁版牎楠屻€侀敊璇槧灏勩€佽緟鍔╅€昏緫
  include/   瀵瑰澶存枃浠?fops.h / fops_types.h
  internal/  FOPS 鍐呴儴鎺ュ彛
  ops/       鍚勭被 OP 鐨勫叿浣撳疄鐜?```

涓撻鏂囨。鎸?`src/fops/ops/` 鐨勬枃浠跺垝鍒嗕负涓伙紝鍙湁 dispatch銆乮dentity銆乫lags銆乪rrors 灞炰簬妯垏璇存槑銆?
## 褰撳墠绾﹀畾

- 鏂囨。銆佹敞閲婂拰璁捐璇存槑浠ヤ腑鏂囦负涓伙紝蹇呰鑻辨枃鏈淇濈暀鑻辨枃鍘熻瘝銆?- 瀵瑰缁熶竴鍏ュ彛浼樺厛璧?`fops_dispatch()`锛涚粏绮掑害鎺ュ彛浠嶄綔涓哄唴閮ㄥ疄鐜板崟鍏冨拰杞婚噺璋冪敤鍏ュ彛銆?- 鍒涘缓绫?OP 鍚屾椂鎻愪緵杞婚噺鎺ュ彛鍜?`*_plus` 鎺ュ彛锛沗*_plus` 鍦ㄥ垱寤烘垚鍔熷悗杩斿洖灞炴€э紝鍑忓皯涓婂眰棰濆 getattr銆?- `lookup_plus` 杩斿洖鐩綍椤硅В鏋愮粨鏋滃拰灞炴€э紱杞婚噺 `lookup` 鍙繑鍥炲璞¤韩浠姐€?- `readdir` 鍙繑鍥炵洰褰曢」鍩虹淇℃伅锛沗readdirplus` 杩斿洖鐩綍椤瑰拰灞炴€с€?- flag 璇箟鐢?`common/flag` 瀹氫箟锛孎OPS 閫氳繃 spec 琛ㄩ檺鍒舵瘡涓?OP 鍙帴鍙楃殑 flag 闆嗗悎銆?
