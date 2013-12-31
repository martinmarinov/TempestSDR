# This makefile will just do make clean and make all to all of the projects

# Make all
all :
	@$(MAKE) -C TSDRPlugin_RawFile/ all JAVA_HOME=$(JAVA_HOME)
	@$(MAKE) -C TSDRPlugin_Mirics/ all
	@$(MAKE) -C TempestSDR/ all
	@$(MAKE) -C JavaGUI/ all

# Clean artifacts
clean :
	@$(MAKE) -C TSDRPlugin_RawFile/ clean
	@$(MAKE) -C TSDRPlugin_Mirics/ clean
	@$(MAKE) -C TempestSDR/ clean
	@$(MAKE) -C JavaGUI/ clean
