## UART Secure design
This application helps protect against unauthorized debug access that could lead to data
hacking. Production software should only be accessible by authorized personnel.
Furthermore, the application leverages the Optiga™ Trust M hardware accelerator to
enhance security operations. The overall application flow is described as follows.


![uart design](/images/uartflow.png)


## First environment setup
Download source code for develop hub.

```c
 git clone https://github.com/tesaiot/developer-hub.git -b uart
 cd examples/uart_secure
 ./apply_patches.sh

```
If the board is new, the UART key will not be updated automatically. You must update the key by modifying the code in the proj_cm33_ns folder.

Change the parameter to PROTECTED_UPDATE in order to update the key.
```c
 crypt_authen_key_test(PROTECT_UPDATED);
```

After program and run completely 

Change the parameter back to RUNTIME to switch to runtime mode.
```c
crypt_authen_key_test(RUNTIME);

```
and program again 