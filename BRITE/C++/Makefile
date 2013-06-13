# Makefile for BRITE 2.0

CC = g++ -Wall
CFLAGS = -o 
MODELS=./Models

brite: BriteMain.o  Node.o Edge.o Graph.o Topology.o Util.o Parser.o \
	Parser.o Model.o RouterModel.o ASModel.o RouterWaxmanModel.o \
	RouterBarabasiAlbertModel.o ASWaxmanModel.o ASBarabasiAlbertModel.o \
	TopDownHierModel.o BottomUpHierModel.o ImportedFileModel.o
	$(CC) $(CFLAGS) brite BriteMain.o Graph.o Topology.o Node.o \
		Edge.o Util.o Parser.o Model.o RouterModel.o \
		ASModel.o RouterWaxmanModel.o RouterBarabasiAlbertModel.o \
		ASWaxmanModel.o ASBarabasiAlbertModel.o \
		TopDownHierModel.o BottomUpHierModel.o ImportedFileModel.o -lm -g

BriteMain.o: BriteMain.cc Brite.h
	$(CC) -c BriteMain.cc -g

Node.o: Node.cc Node.h
	$(CC) -c Node.cc -g

Edge.o: Edge.cc Edge.h
	$(CC) -c Edge.cc -g

Util.o: Util.cc Util.h
	$(CC) -c Util.cc -g

Graph.o: Graph.cc Graph.h
	$(CC) -c Graph.cc -g

Topology.o: Topology.cc Topology.h
	$(CC) -c Topology.cc -g

Parser.o: Parser.cc Parser.h
	$(CC) -c Parser.cc -g

Model.o: $(MODELS)/Model.cc $(MODELS)/Model.h
	$(CC) -c $(MODELS)/Model.cc -g

RouterModel.o: \
	$(MODELS)/RouterModel.cc $(MODELS)/RouterModel.h
	$(CC) -c $(MODELS)/RouterModel.cc -g

RouterWaxmanModel.o: \
	$(MODELS)/RouterWaxmanModel.cc $(MODELS)/RouterWaxmanModel.h 
	$(CC) -c $(MODELS)/RouterWaxmanModel.cc -g

RouterBarabasiAlbertModel.o: \
	$(MODELS)/RouterBarabasiAlbertModel.cc $(MODELS)/RouterBarabasiAlbertModel.h
	$(CC) -c $(MODELS)/RouterBarabasiAlbertModel.cc -g

ASModel.o: \
	$(MODELS)/ASModel.cc $(MODELS)/ASModel.h
	$(CC) -c $(MODELS)/ASModel.cc -g

ASWaxmanModel.o: \
	$(MODELS)/ASWaxmanModel.cc $(MODELS)/ASWaxmanModel.h 
	$(CC) -c $(MODELS)/ASWaxmanModel.cc -g

ASBarabasiAlbertModel.o: \
	$(MODELS)/ASBarabasiAlbertModel.cc $(MODELS)/ASBarabasiAlbertModel.h
	$(CC) -c $(MODELS)/ASBarabasiAlbertModel.cc -g

TopDownHierModel.o: \
	$(MODELS)/TopDownHierModel.cc $(MODELS)/TopDownHierModel.h
	$(CC) -c $(MODELS)/TopDownHierModel.cc  -g	

BottomUpHierModel.o: \
	$(MODELS)/BottomUpHierModel.cc $(MODELS)/BottomUpHierModel.h
	$(CC) -c $(MODELS)/BottomUpHierModel.cc  -g	

ImportedFileModel.o: \
	$(MODELS)/ImportedFileModel.cc $(MODELS)/ImportedFileModel.h
	$(CC) -c $(MODELS)/ImportedFileModel.cc  -g	

clean:
	rm -f *.o *~
	rm -f brite
