# pyasm
Write a simple driver and python client that lets me run prievilieged opcodes from the Windows kernel such as: rdmsr, etc...

# Dependencies
	pip install pywin32 construct ipython pyreadline
	
# Usage
	To use this library first run the following command and reboot:
		bcdedit /set testsigning on
	
	Right click the "pyasm.inf" file and press install
	Once you "sc start pyasm" you can start using "pyasm.py" to communicate with driver and perform prievilieged opcodes from kernel
	