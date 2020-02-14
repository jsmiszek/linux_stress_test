all: directory makeprograms linkprograms

directory:
	mkdir -p ODP

makeprograms:
	$(MAKE) -C massivereader; $(MAKE) -C multiwriter

linkprograms:
	ln -s ../massivereader/massivereader ODP/massivereader ; ln -s ../multiwriter/multiwriter ODP/multiwriter

clean:
	$(MAKE) -C massivereader clean; $(MAKE) -C multiwriter clean; rm -rf ODP
