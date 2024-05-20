set ldflags {}

if {[info exists ::env(LDFLAGS)]} {
  lappend ldflags {*}"$::env(LDFLAGS)"
}

