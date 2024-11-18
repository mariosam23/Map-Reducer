### Description
&emsp; An app that allows users to rapidly find where a specific word appears across a large dataset of text files.
It uses efficiently multiple threads resulting in a speedup of **3x** (*for 4 mappers and 4 reducers*) compared to a single-threaded approach.


---

### Installation and Usage Instructions
1. Clone the Repository
2. Build the Application
	- `make`
3. Run the Application
    - `./map-reducer <number of mappers> <number of reducers> <input_file>`
4. See the results in *.txt files. For example, `a.txt` contains all the words that start with 'a', followed by the file ids where they appear.
---

### Stage 1: Map
- I've created a thread safe queue that holds the file names and file ids.
- Each mapper thread is busy reading the content of the files popped from queue. This way, the work is distributed as evenly as possible between the mappers.
- Every mapper will store in a map the words and the file ids where they appear.

---

### Stage 2: Reduce
- Synchronization between the mappers and the reducers is done using a barrier that waits for **num_mappers** + **num_reducers** threads. This way, reducers threads will start only after all the mappers have finished their work.
- Every reducer is responsible for a specific set of letters.
- For each letter, the reducer will merge the results from all the mappers based on the first letter of the word.
- Vector resulted from the merge is sorted.
- Reducers will write in the files called `<reducer_letter>.txt` the words and the file ids where they appear.

---

<br>

<h5> &copy; 2024 Mario Sampetru</h5>
