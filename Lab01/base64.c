#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int N = 1024;

// Function to perform Base64 encoding on an input string
char* base64_encode(char* inputString) {
    int len = strlen(inputString);
    char set64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; 

    char *ans = (char *) malloc(2 * N * sizeof(char)); 

    int numBits = 0, pad = (3 - len % 3) % 3;
    int val = 0;
    int resIndex = 0;

    // Iterate through the input string in chunks of 3 bytes
    for (int i = 0; i < len; i += 3) {
        val = 0;
        numBits = 0;
        for (int j = i; j <= i + 2 && j < len; j++) {
            val = val << 8;
            val = val | inputString[j];
            numBits += 8;
        }

        // Adjust the number of bits and padding if necessary
        if (numBits == 8) {
            val = val << 4;
            numBits = 12;
        } else if (numBits == 16) {
            val = val << 2;
            numBits = 18;
        }

        // Encode the chunks in groups of 6 bits and append to the result
        for (int j = 6; j <= numBits; j += 6) {
            int temp = val >> (numBits - j);
            int index = temp % 64;
            char ch = set64[index];
            ans[resIndex++] = ch;
        }
    }

    // Add padding characters
    for (int j = 0; j < pad; j++) {
        ans[resIndex++] = '=';
    }
    ans[resIndex] = '\0';

    return ans;
}

// Function to perform Base64 decoding on an input string
char* base64_decode(char* inputString) {
    int len = strlen(inputString); 

    char *ans = (char *) malloc(N * sizeof(char)); 

    int numBits = 0;
    int val = 0;
    int resIndex = 0;
    int count = 0;

    // Iterate through the input string in chunks of 4 characters
    for (int i = 0; i < len; i += 4) {
        val = 0;
        numBits = 24;
        count = 0;
        for (int j = i; j <= i + 3 && j < len; j++) {
            // Determine the index in the Base64 set
            int temp_index;
            if (inputString[j] >= 'A' && inputString[j] <= 'Z')
                temp_index = inputString[j] - 'A';
            else if (inputString[j] >= 'a' && inputString[j] <= 'z')
                temp_index = inputString[j] - 'a' + 26;
            else if (inputString[j] >= '0' && inputString[j] <= '9')
                temp_index = inputString[j] - '0' + 52;
            else if (inputString[j] == '+')
                temp_index = 62;
            else if (inputString[j] == '/')
                temp_index = 63;
            else if (inputString[j] == '=')
            {
                count++;
                continue;
            }
            val = val << 6;
            val = val | temp_index;
        }

        // Adjust the number of bits based on padding count
        if (count == 2) {
            val = val >> 4;
            numBits = 8;
        } else if (count == 1) {
            val = val >> 2;
            numBits = 16;
        }

        // Decode the chunks in groups of 8 bits and append to the result
        for (int j = 8; j <= numBits; j += 8) {
            int temp = val >> (numBits - j);
            int index = temp % 256;
            ans[resIndex++] = (char) index;
        }
    }
    ans[resIndex] = '\0';
    return ans;
}
