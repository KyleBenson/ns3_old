#BRITE Makefile. 

all:	c++ java gui  
 
clean:  c++clean javaclean guiclean	

c++:
	@if test -f C++/Makefile; then\
	 (cd C++; make) ; \
	fi

c++clean:
	@if test -f C++/Makefile; then\
	 (cd C++; make clean); \
	fi

java:
	@if test -f Java/Makefile; then \
	  (cd Java; make) ; \
	fi

javaclean:
	@if test -f Java/Makefile; then\
	  (cd Java; make clean) ;\
	fi

gui:
	@if test -f GUI/Makefile; then \
	 (cd GUI; make) ; \
	fi

guiclean: 
	@if test -f GUI/Makefile; then\
	  (cd GUI; make clean ); \
	fi
