set target cosim

source open_project.tcl
source common_cflags.tcl
source common_csimflags.tcl
source common_cosimflags.tcl
source ldflags.tcl
source add_files.tcl
source add_tb_files.tcl

source config_rtl.tcl
source config_dataflow.tcl
source config_interface.tcl
csynth_design
cosim_design -ldflags $ldflags -trace_level [gets [open [file join .. TRACE_LEVEL] r]]

puts {---------------------------------------------}
puts " PRJ_DIR: $prj_dir"
puts {---------------------------------------------}
exit
