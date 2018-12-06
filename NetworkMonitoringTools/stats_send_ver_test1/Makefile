object = main.o dump.o get_adapter.o \
	get_port_info.o get_link_stats.o \
	trap.o list_handle.o ntp.o get_host_info.o \
	local_timer.o local_comm.o

CFLAGS = -lpthread -lrt

module_stats : $(object)
	cc -o module_stats $(object) $(CFLAGS)

$(object) : main.h
dump.o get_adapter.o get_port_info.o: getnet.h
trap.o get_port_info.o: trap.h 

.PHONY : clean
clean :
	-rm module_stats $(object)
