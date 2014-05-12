
dummy::
	cd src/bin; $(MAKE) depend; $(MAKE)

install::
	cd src/bin; $(MAKE) install

uninstall::
	cd src/bin; $(MAKE) uninstall

clean::
	cd src/bin; $(MAKE) clean
	cd msw_package; $(MAKE) clean

distclean::
	cd src/bin; $(MAKE) distclean
	cd msw_package; $(MAKE) distclean
	-rm config.cache config.log config.status
