# Extracts the <dir>/<file> pairs from the *_src lists in makefile.gaz.
# Used by gen_game_sources.sh; keep in sync with nothing - makefile.gaz is
# the single source of truth this reads.
/^[a-z]+_src[ \t]*:=/ {
  dir=$1; sub(/_src.*/,"",dir);
  collecting = ($0 ~ /\\[ \t\r]*$/);
  line=$0; sub(/^[^=]*=/,"",line); gsub(/\\/,"",line); gsub(/\r/,"",line);
  n=split(line,a,/[ \t]+/); for(i=1;i<=n;i++) if(a[i]!="") print dir "/" a[i];
  next
}
collecting {
  cont = ($0 ~ /\\[ \t\r]*$/);
  line=$0; gsub(/\\/,"",line); gsub(/\r/,"",line);
  n=split(line,a,/[ \t]+/); for(i=1;i<=n;i++) if(a[i]!="") print dir "/" a[i];
  if(!cont) collecting=0
}
