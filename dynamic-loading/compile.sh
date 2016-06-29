rm *.so

gcc libapple.c -fPIC -shared -o libapple.so
gcc libbanana.c -fPIC -shared -o libbanana.so
gcc liborange.c -fPIC -shared -o liborange.so

gcc main.c -o main

./main

