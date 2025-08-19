# Command

```sh
valgrind feral testdir tests
```

# Output

```sh
==437586==
==437586== HEAP SUMMARY:
==437586==     in use at exit: 10,133 bytes in 6 blocks
==437586==   total heap usage: 36,767 allocs, 36,761 frees, 5,346,565 bytes allocated
==437586==
==437586== LEAK SUMMARY:
==437586==    definitely lost: 0 bytes in 0 blocks
==437586==    indirectly lost: 0 bytes in 0 blocks
==437586==      possibly lost: 0 bytes in 0 blocks
==437586==    still reachable: 10,133 bytes in 6 blocks
==437586==         suppressed: 0 bytes in 0 blocks
==437586== Rerun with --leak-check=full to see details of leaked memory
==437586==
==437586== For lists of detected and suppressed errors, rerun with: -s
==437586== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```