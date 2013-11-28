all : jnilib java

java :
#TODO! Implement java compilation from here
	echo Not implemented

jnilib :
	@$(MAKE) -C jni/ all

cleanjni:
	@$(MAKE) -C jni/ clean
	
clean : cleanjni
	rm -rf bin/