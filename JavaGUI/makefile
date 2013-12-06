all : jnilib java

java :
	rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))
	javafiles=$(call rwildcard,/src/,*.java)
	
jnilib :
	@$(MAKE) -C jni/ all

cleanjni:
	@$(MAKE) -C jni/ clean
	
clean : cleanjni
	rm -rf bin/