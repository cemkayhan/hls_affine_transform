set common_csynthflags {}

if {[info exists ::env(CSYNTHFLAGS)]} {
  lappend common_csynthflags {*}"$::env(CSYNTHFLAGS)"
}
