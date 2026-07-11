# FOPS Dispatch 缁熶竴鍏ュ彛

`fops_dispatch(args)` 鏄?FOPS 闈㈠悜涓婂眰鐨勭粺涓€鍏ュ彛銆備笂灞傚彧闇€瑕佹瀯閫?`fops_args_t`锛屽～鍐?`op`銆佸叕鍏卞瓧娈靛拰瀵瑰簲 union 鍒嗘敮锛屽啀浜ょ粰 dispatch 鎵ц銆?
## 璋冪敤娴佺▼

```text
fops_dispatch(args)
  鈫?fops_validate_args(args)
  鈫?鏌?fops_op_spec_t
  鈫?鏍￠獙 op / flag / 鍩虹鎸囬拡 / name 瑙勫垯
  鈫?g_fops_ops[op](args)
  鈫?璋冪敤鐜版湁 fops_lookup/create/... 缁嗙矑搴﹀嚱鏁?```

璇ユ祦绋嬫妸鈥滃叆鍙ｇ粺涓€鈥濆拰鈥滃疄鐜板彲缁存姢鈥濆垎寮€锛氬閮ㄧ湅鍒扮粺涓€鍑芥暟鎸囬拡妯″瀷锛屽唴閮ㄤ粛淇濈暀娓呮櫚鐨勫崟 OP 瀹炵幇銆?
## fops_args_t 璁捐鍘熷垯

`fops_args_t` 鐨勮璁＄洰鏍囨槸閬垮厤澶栭儴鎺ュ彛闅忕潃 OP 鍙傛暟鑶ㄨ儉鑰屽け鎺с€?
鍏叡瀛楁閫傚悎鏀惧湪澶栧眰锛?
- `op`锛氭搷浣滃瓧銆?- `flags`锛歄P flag銆?- `parent_fuid`锛氬ぇ澶氭暟 name-based OP 鐨勭埗鐩綍銆?- `target_fuid`锛氱洿鎺ヤ綔鐢ㄤ簬瀵硅薄鐨?OP銆?- `name`锛氱洰褰曢」鍚嶇О鎴?xattr 鍚嶇О銆?- `out_attr` / `out_fuid` / `out_count` 绛夊父瑙佽緭鍑烘寚閽堛€?
宸紓杈冨ぇ鐨勫弬鏁版斁鍦?union 鍒嗘敮涓紝渚嬪 create銆乺ename銆乺w銆亁attr銆乻tatfs 绛夈€?
## OP spec 琛?
`fops_op_spec_t` 鏄?dispatch 鐨勮鍒欐簮锛岃嚦灏戞弿杩帮細

- OP 鏄惁瀛樺湪瀹炵幇銆?- 鍏佽鐨?flag 闆嗗悎銆?- 浜掓枼 flag 闆嗗悎銆?- 鏄惁闇€瑕?parent/name/target銆?- name 鏄惁鍏佽 `.` 鍜?`..`銆?- 鏄惁闇€瑕佽緭鍏ョ紦鍐插尯銆佽緭鍑虹紦鍐插尯鎴栬姹傜粨鏋勪綋銆?
杩欐牱鍋氱殑濂藉鏄細鏂板 OP 鏃跺厛琛?spec锛屽啀琛ュ疄鐜帮紱璋冪敤瑙勫垯鍙互闆嗕腑瀹℃煡锛屼笉闇€瑕佹暎钀藉湪姣忎釜 OP 涓弽澶嶅啓鐩稿悓鍒ゆ柇銆?
## 缁嗙矑搴︽帴鍙?
淇濈暀 `fops_lookup()`銆乣fops_create()` 绛夌粏绮掑害鎺ュ彛鏄繀瑕佺殑銆傚畠浠敤浜庯細

- dispatch 鍐呴儴澶嶇敤銆?- 鍗曞厓娴嬭瘯鐩存帴瑕嗙洊鏌愪釜 OP銆?- 妯″潡鍐呴儴杞婚噺璋冪敤锛岄伩鍏嶅繀椤绘瀯閫犲畬鏁?args銆?
绾﹀畾锛氱粏绮掑害鎺ュ彛涔熷繀椤婚伒瀹堝悓涓€璇箟锛屼笉鍏佽缁曞紑鏍稿績瀹夊叏妫€鏌ヤ骇鐢熶笉涓€鑷磋涓恒€?
