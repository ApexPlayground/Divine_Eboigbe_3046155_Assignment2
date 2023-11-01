// Distributed Systems - 2023
// Assignment 2 - template
// TODO: Divine Eboigbe - 3046155

#include <iostream>
#include <fstream>
#include <mpi.h>
#include <string>

//Global variables
const int arraySize = 480;
int world_size, world_rank;
int key;
char totalArray[arraySize];
int actualsize = 0;


//createData helper method - Do not modify
void createData(int rank) {
    /* DATA METHOD
    This method populates an array with the text read from file.
    */

    // File must be saved in the same directory as the executable of this program
    std::string nodeFileName = "ciphertext.txt"; //read your corresponding .txt file
    std::ifstream MyReadFile(nodeFileName);
    // Only 1 node need read the file, output to console
    std::cout << "Node " << rank << "  reading file: " << nodeFileName << std::endl;

    // Use a while loop together with the getline() function to read the file line by line
    int i = 0;
    while (!MyReadFile.eof()) {
        // add text from file into array
        MyReadFile.getline(totalArray, arraySize);
        actualsize++;
    }
    // Close the file
    MyReadFile.close();
}
//decryptText helper method - Do not modify
char* decryptText(int key, char* arr, int size) {
    int length, i;
    char c;
    //loop through each string in array
    for (int j = 0; j < size; j++) {
        c = arr[j];
        //if character is in alphabet, convert to uppercase
        if (isalpha(c)) {
            c = toupper(c);
            //decrypt using formula (c - key % 26)
            c = 'A' + (((c - 'A') - key + 26) % 26);
            arr[j] = c;
        }
    }
    return arr;
}
//searchText helper method - Do not modify
int searchText(char* arr, int size, const char* needle) {
    int hits = 0;
    char* found = arr;
    //search given array for the needle
    while ((found = strstr(found, needle)) != NULL && found - arr < size) {
        hits++;
        found++;
    }
    return hits;
}
//exportText helper method - Do not modify
void exportText(char* arr, int size, const char* filename) {
    std::ofstream outfile(filename); //create new file
    outfile.write(arr, size); //write array to file
    outfile.close(); //close the file
}


int main(int argc, char** argv) {
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // Calculate the chunk size for each node
    int chunkSize = arraySize / world_size;
    // Create an array for each node to store its chunk
    char* chunk = new char[chunkSize];



    if (world_rank == 0) {
        // Prompt the user to enter the decryption key (1-25)
        do {
            std::cout << "Enter the decryption key (1-25): ";

            // Store user input
            std::string input;
            std::cin >> input;

            // Check if the input is numeric
            if (isdigit(input[0])) {
                // Convert the inputted string to an integer
                int numericKey = std::stoi(input);
                if (numericKey >= 1 && numericKey <= 25) {
                    key = numericKey;
                }
                else {
                    std::cout << "Invalid key. Please enter a key in the range 1-25." << std::endl;
                }
            }
            else {
                std::cout << "Invalid key. Please enter a numeric key in the range 1-25." << std::endl;
            }
        } while (key < 1 || key > 25);

       
    }

    // Broadcast the decryption key from Node 0 to all other nodes
    MPI_Bcast(&key, 1, MPI_INT, 0, MPI_COMM_WORLD);

    //Barrier to synchronize processes
    MPI_Barrier(MPI_COMM_WORLD);


    // Read the file on npode 0 
    if (world_rank == 0) {
        createData(world_rank);
    }

    //Barrier to synchronize processes
     MPI_Barrier(MPI_COMM_WORLD);

 

    // Scatter the data from Node 0 to all nodes
    MPI_Scatter(totalArray, chunkSize, MPI_CHAR, chunk, chunkSize, MPI_CHAR, 0, MPI_COMM_WORLD);

    // Decrypt the chunk
    char* decipheredArray = decryptText(key, chunk, chunkSize);

    // Output the deciphered array for each node
    std::cout << "Node " << world_rank << "'s deciphered array: " << decipheredArray << std::endl;
   
    std::cout << " " << std::endl;

    //Barrier to synchronize processes
    MPI_Barrier(MPI_COMM_WORLD);


    // Count occurrences of "DISTRIBUTED" in the decipheredArray
    const char* searchString = "DISTRIBUTED";
    int hits = searchText(decipheredArray, chunkSize, searchString);
    std::cout << "Instances of  " << searchString << " found in node " << world_rank  << " is " << hits << std::endl;

     std::cout << " " << std::endl;

     MPI_Barrier(MPI_COMM_WORLD);
    //initialize totalhits
    int totalhits = 0;

    // Collect hits from all nodes on Node 3
    MPI_Reduce(&hits, &totalhits, 1, MPI_INT, MPI_SUM, 3, MPI_COMM_WORLD);

    
    //Send totalhits info to node 2
    if (world_rank == 3) {
        MPI_Send(&totalhits, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
    }

    //Receive info from node 3 
    int receivedTotalHits = 0;
    if (world_rank == 2) {
        MPI_Recv(&receivedTotalHits, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        std::cout << "Distributed was found a total of " << receivedTotalHits << " times." << std::endl;
    }

    // Create an array to store the partial decrypted arrays
    char* partialDecryptedArray = new char[chunkSize * world_size];

    // Gather partial decrypted arrays from all nodes to Node 0
    MPI_Gather(decipheredArray, chunkSize, MPI_CHAR, partialDecryptedArray, chunkSize, MPI_CHAR, 0, MPI_COMM_WORLD);

    if (world_rank == 0) {
        // Output the reconstructed array for Node 0

        std::cout << " " << std::endl;

        std::cout << "Node " << world_rank << "'s reconstructed array: " << partialDecryptedArray << std::endl;

        std::cout << " " << std::endl;

        // Export the reconstructed data to a file on Node 0
        exportText(partialDecryptedArray, arraySize, "DivineDecryptedText.txt");
    }
    MPI_Finalize();
    return 0;
}