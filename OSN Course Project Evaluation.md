---
title: OSN Course Project Evaluation

---

# OSN Course Project Evaluation

## Some notes

- Only POSIX libraries are allowed (show this to me in your code)
- Only 3 executables must be run (NS, SS, Client). However the client and SS will ofcourse run in multiple terminals.
- Changing the directory structure will lead to a penalty.
- Please look at the individual sheet as well. This includes their individual Viva, contribution, etc. Contribution is based on how you feel + teammate's feedback

## Initialisation Section

### Multiple Device Support

Ask them to arrange 4 laptops on their own. Dedicate 1 laptop for NM, 2 for SS and 1 for client. On laptops on which SS are opened make sure that odd SS go on 1 laptop and even SS on another laptop. Then start with the eval.
Note:- If they say that multiple device support is not there Penalty of *30 Marks*.

### Initialise naming server 

```bash
# Open a new terminal
# start running the Naming Server
```

Only one NM should be running at any given moment.

- Show that the NM starts working. (5 marks)

### Connecting storage servers

Open terminals in the following directories, run your storage server executable and give the given paths as the accessible paths

_SS1_

`root_directory/dir_pbcui`

    /dir_bzaca
    /dir_bzaca/file_ofr.txt
    /dir_bzaca/file_qsw.txt
    /dir_bzaca/file_uag.txt
    /dir_bzaca/dir_ccftl/dir_hbsfj
    /dir_qkdez/dir_htcrv/dir_mrrcz
    /dir_qkdez/dir_htcrv/dir_mrrcz/file_imz.txt

_SS2_

`root_directory/dir_gywnw/dir_fzxpq`

    /dir_rzuni/dir_eqfej
    /dir_rzuni/dir_eqfej/file_wux.txt
    /dir_rzuni/dir_aqmhk



_SS3_

`root_directory/dir_vhfih/dir_lhkzx`

    /dir_ctpcj
    /dir_ctpcj/dir_apsck
    /dir_lhkzx/dir_ctpcj/dir_apsck/file_hwa.txt
    /dir_ctpcj/dir_apsck/file_rkn.txt
    /dir_ctpcj/dir_hfccx
    /dir_ctpcj/dir_hfccx/here.txt


_SS4_ 

`root_directory/dir_psjio/dir_ctyuy`

    /dir_bqays
    /dir_bqays/dir_adecw/file_qdh.txt
    /dir_bqays/dir_hlxuh
    /dir_bqays/dir_hlxuh/file_xmu.txt
    /dir_bqays/dir_hlxuh/file_rwf.txt
    /dir_bqays/dir_hlxuh/file_zsb.txt


_SS5_

`root_directory/dir_psjio/dir_kgabk/dir_jtngu`

    /dir_sptuh
    /dir_sptuh/file_nda.txt
    /dir_sptuh/file_ytw.txt

_SS6_

`root_directory/dir_vymdo`

    /dir_pfaau
    /dir_pfaau/student_song
    /dir_pfaau/student_song/your_song.mp3
    /dir_pfaau/coldplay.mp3
    /dir_pfaau/dir_cscou
    /dir_pfaau/dir_cscou/dir_mbbdr
    /dir_pfaau/dir_cscou/dir_mbbdr/file_mpe.txt
    /dir_pfaau/dir_cscou/dir_rjmdf
    /dir_pfaau/dir_cscou/dir_rjmdf/file_kdx.txt
    /dir_pfaau/dir_fpkwl
    /dir_pfaau/dir_fpkwl/dir_cgpqn
    /dir_pfaau/dir_fpkwl/dir_gxueg
    /dir_picqp/dir_odqtp
    /dir_picqp/dir_picaz/dir_cdsdh
    /dir_picqp/dir_picaz/dir_cdsdh/file_dar.txt
    /dir_picqp/dir_picaz/dir_cdsdh/file_yyj.txt
    /dir_picqp/dir_picaz/dir_zdoid
    /dir_picqp/dir_picaz/dir_tnaqz
    /dir_picqp/dir_picaz/dir_tnaqz/file_kox.txt

- Show that SS1 and SS2 connect. 10 Marks
- Then show that SS3-SS6 connect. 35 Marks

### Connecting clients

Open 5 different terminals and run the client executable

Two cases:
- Only one client clients - 3 Marks
- Other 4 clients also connect - 7 Marks

## Operations

*Run each of the cases and show that it works*

### List

Should return all accessible paths present with the NS - 10 Marks.

### Reading

1. The client asks to read `/dir_ctpcj/dir_hfccx/here.txt`
2. The NM returns the port of SS3
3. The client communicates with SS3
4. The file's contents are printed (`cat root_directory/dir_vhfih/dir_lhkzx/dir_ctpcj/dir_hfccx/here.txt` if you want to see what's inside)
5. The client stops communicating with SS3

If read succesful - 25 Marks. 

**Error Testing:**

1. repeat the same steps as above for reading `/dir_picqp/dir_picaz/dir_tnaqz/file_lcchehe.txt`

2. repeat the same steps for reading `/dir_picqp/dir_picaz/dir_tnaqz`

3. repeat the same steps for reading `/dir_veyiq/dir_aiilb/file_cqe.txt`.
(Since no directory in this was present in accessible paths. This should give error not found.)

Each testcase penalty - 5 marks.

### Writing

1. The client asks to read `/dir_pfaau/dir_cscou/dir_mbbdr/file_mpe.txt`
2. The NM returns the port of SS6
3. The client communicates with SS6
4. Make a few changes (you might need to pass the required changes in step 1 if they have implemented it like that)
5. The file's contents are changed (`cat root_directory/dir_vymdo/dir_pfaau/dir_cscou/dir_mbbdr/file_mpe.txt` if you want to see what's inside)
6. The client stops communicating with SS6

If works award 25 Marks.

**Error Testing:**
1. repeat the same steps for writing `/dir_picqp/dir_picaz/dir_tnaqz/file_lcchehe.txt`
2. repeat the same steps for writing `/dir_picqp/dir_picaz/dir_tnaqz`

Penalty - 5 per testcase

### Getting info about files

1. The client asks to read `/dir_pfaau/dir_cscou/dir_mbbdr/file_mpe.txt`
2. The NM returns the port of SS6
3. The client communicates with SS6
4. The file's perms are printed (`ls root_directory/dir_vymdo/dir_pfaau/dir_cscou/dir_mbbdr/file_mpe.txt` if you want to see the perms)
5. The client stops communicating with SS6
6. Manually change the perms of the file using `chmod` and then repeat steps 1-5 and show that the changes are reflected.

**for directories:**
- Now, repeat the same steps for reading `/dir_picqp/dir_picaz/dir_tnaqz`

5 marks for files and 5 marks for directories.

**Error Testing:**
1. repeat the same steps for reading `/dir_picqp/dir_picaz/dir_tnaqz/file_lcchehe.txt`

Penalty of 5 if error testing fails.

### Streaming Audio Files

**Case 1:** - 25 Marks
1. The client asks to stream `/dir_pfaau/coldplay.mp3`.
2. The NM returns the port of SS6
3. The client communicates with SS6
4. Verify the implementation on the client side to determine how the music playback is being handled. Specifically, check whether the client is accumulating all packets before starting playback or if it is receiving packets and playing the music simultaneously. Ideally, the latter approach (receiving and playing concurrently).
5. The client stops communicating with SS6

**Case 2: (For TA to judge them)**
1. The client asks to stream `/dir_pfaau/student_song/your_song.mp3`.
2. The NM returns the port of SS6
3. The client communicates with SS6
4. Verify the implementation on the client side to determine how the music playback is being handled. Specifically, check whether the client is accumulating all packets before starting playback or if it is receiving packets and playing the music simultaneously. Ideally, the latter approach (receiving and playing concurrently).
5. The client stops communicating with SS6

(Mark out of 5 how much you liked the music.)
(If they did not add a file with this name don't give any marks.)

**Error Handling**

1. The client asks to stream `/dir_pfaau/dir_cscou/dir_mbbdr/file_mpe.txt`. Error for invalid file type should come. 
2. The client asks to stream `/dir_veyiq/05-coldplay-yellow.mp3`. Since it is not an accessible path no streaming should be done

Penalty of 5 marks per case.

Note:- In case they cannot handle mp3 files. Ask them which extension they have done it for and test accordingly by getting a new audio file in that extension.

### Creating

#### Task 1 - 10 Marks
1. Client asks to create a file `whats.txt` in `/dir_bqays/dir_hlxuh`
2. NM figures out that the path is in SS4
3. NM creates the file
4. NM informs the client that the file has been created.

**Parallel Commands:**

One client should send the first request and another client the second.

#### Task 2 - 5 Marks
1. Client asks to create a directory `tempqqh` in `/dir_bqays/dir_hlxuh`
2. NM figures out that the path is in SS4
3. NM creates the directory
4. NM informs the client that the file has been created.

#### Task 3 - 15 Marks.
1. Client asks to create a file `hi.txt` in `/dir_bqays/dir_hlxuh`
2. NM figures out that the path is in SS4
3. NM creates the file
4. NM informs the client that the file has been created.

**Error Testing:**

1. Client asks to create a directory `tempqqh2` in `/dir_bqays/dir_hlxuh/hi.txt`. See that the path is a file so directory cannot be created. NM/SS or client should not crash.
2. Client tries to create a directory `OSN_Sucks` in `/dir_veyiq/dir_aiilb`. Given base path is not a accessible path so an error should be given.

Penalty - 5 marks per testcase.

### Deleting

**Case 1:** - 10 Marks
1. Client asks to delete the file `/dir_bqays/dir_hlxuh/whats.txt`
2. NM figures out that the path is in SS4
3. NM deletes the file
4. NM informs the client that the file has been deleted.

**Case 2:** - 10 Marks
1. Client asks to delete the folder `/dir_rzuni/dir_aqmhk`
2. NM figures out that the path is in SS2
3. NM deletes the directory
4. NM informs the client that the file has been deleted.

**Case 3:** - 10 Marks
1. Client asks to delete directory `/dir_bqays/dir_hlxuh/tempqqh`
2. NM figures out that the path is in SS4
3. NM delets the directory
4. NM informs the client that the file has been created.

**Important:**
Show that the copying and deletion are not being done using absolute paths in the naming server. The NS should tell the SS's to do the needful.

**Error Testing**
1. Client asks to delete the file `/dir_bqays/dir_hlxuh/whats.txt`
2. The given path is not present now since it was deleted in Case 1. Should given an error of `FILE NOT FOUND`.

Penalty - 5 marks per testcase.

### Copying

Before everything, connect SS7 and SS8:

_SS7_

`root_directory/a_song_ice_fire`

    /plot_dir
    /plot_dir/plot.txt

_SS8_

`root_directory/gravity_falls`

    /gravity_subdir
    /gravity_subdir/intro.txt
    /gravity_subdir/so-good.webp
    /gravity_subdir2

*Both of them should connect*

If the two new SS don't connect, run the following with some other file and non-empty directories.

**Case 1:** - 10 Marks
1. Client asks to copy `/plot_dir/plot.txt` to `/gravity_subdir`
2. NM notices that `/plot_dir/plot.txt` is in SS7
3. NM copies plot.txt to SS8's `/gravity_subdir` directory
4. NM informs client
5. `/gravity_subdir/plot.txt` should have been created

**Case 2:** - 15 Marks
1. Client asks to copy the directory `/dir_bqays/dir_hlxuh` to `/gravity_subdir`
2. NM notices that `/dir_bqays/dir_hlxuh` is in SS8
3. NM copies the directory and all the _ACCESSIBLE_ paths within it to SS8's `/gravity_subdir` directory
4. NM informs client
5. Show whether copying has been done correctly

**Case 3:** - 5 Marks 
1. Client asks to copy `/gravity_subdir/intro.txt` to `/gravity_subdir2`
2. NM notices that `/gravity_subdir/intro.txt` is in SS8
3. NM copies intro.txt to SS8's `/gravity_subdir2` directory
4. NM informs client
5. Show if the necessary has been done

## Other specs


### Multiple Clients

Note: Test on 1 laptop on which client is being done and other laptop take SS one.
In case multiple device support is not their. Write the commands in all of their terminals and hit enter super fast on all of them.

**Case 1:** - 15 Marks
1. Ask 2 clients to read 2 different files at once.
2. All should be able to read the file.

**Case 2:** - 25 Marks
1. Ask 2 clients to read the same file at once.
2. All should be able to read the file.

**Case 3:** - 15 Marks
1. Ask 2 clients to write the same file at once 
2. Only one of them should be able to.

**Case 4:** - 15 Marks
1. Ask 2 clients to write the same file and 1 clients to read from it at once.
2. Only one of them should be able to write but both the readers should either be able to read or shown a message that they can't.

### Error codes - 20 Marks

These can be numeric, alphanumeric, alphabetical or even entire phrases.

- Show the definitions
- Show the part of the code where they are comminicated back to the client.

### Bookkeeping - 20 Marks

1. Logging and Message Display: Implement a logging mechanism where the NM records every request or acknowledgment received from clients or Storage Servers. Additionally, the NM should display or print relevant messages indicating the status and outcome of each operation. This bookkeeping ensures traceability and aids in debugging and system monitoring. 10 Marks

- show that you are logging neccessary info in the NM

IP Address and Port Recording: The log should include relevant information such as IP addresses and ports used in each communication, enhancing the ability to trace and diagnose issues. 10 Marks

1. Whenever an SS connects, its port and IP address (NOTE: it's fine if it's only the port) are stored in the log/printed by the NM

- show that you are logging the above.

### Search in Naming Server

1. Search optimisation - 50 Marks
2. LRU Cache - 30 Marks

- show code for both functionalities.

### Backup

/dir_ctpcj/dir_apsck/file_rkn.txt - SS3

SSx and SSy - backup servers for SS3.

- Close SS3
    - Client should be able to read `/dir_ctpcj/dir_apsck/file_rkn.txt`. 20 marks
    - Client should be not able to write `/dir_ctpcj/dir_apsck/file_rkn.txt`. 20 marks
- Close SSx
    - client should be able to read `/dir_ctpcj/dir_apsck/file_rkn.txt` 10 marks

- Close SSy
    - client should NOT be able to read `/dir_ctpcj/dir_apsck/file_rkn.txt` 20 marks


### Redundancy

1. Bring SS3 back online with updated accessible path (add a new file which is accessible). It should get backed up in SSx and SSy. - 25 Marks.

2. Close SSx create a new file in SS3, bring back SSx, it should get backed up. - 25 Marks.

Note : If they ensure that after SSx is down, file gets backed up in some other storage server apart from SSy, that's fine too. But at any instant, each added file should be backed up in 2 places.

**Important:**
- Show in your code that you are not using absolute paths.
- Show that the server goes offline when asked to. 

### Copying cont.

Finally, 

1. Close all the SS
2. Close all the clients
3. Close the NM.
4. Start the NM.
5. Connect SS1.
6. Connect SS9 (given below)

_SS9_ : small stress test SS for copying

`root_directory/dir_gywnw/dir_fzxpq/dir_wewny`

    /dir_juqnm/file_pzb.txt
    /dir_juqnm/file_veg.txt
    /dir_juqnm/file_vpx.txt
    /dir_juqnm/big_dir_test
    /dir_juqnm/big_dir_test/big_file_70MB.txt
    /dir_juqnm/big_dir_test/med_file_10MB.txt
    /dir_juqnm/big_dir_test/med_file_15MB.txt

- I should be able to copy /dir_juqnm/big_dir_test/med_file_10MB.txt to anywhere is SS1 - 13 Marks
- I should be able to copy /dir_juqnm/big_dir_test/med_file_15MB.txt to anywhere is SS1 - 10 Marks
- I should be able to copy /dir_juqnm/big_dir_test/big_file_70MB.txt to anywhere is SS1 - 7 Marks

### Async Writing

- Test by sending a large file through client eg: `root_directory/dir_gywnw/dir_fzxpq/dir_wewny/dir_juqnm/big_dir_test/med_file_10MB.txt`. (Copy it's content if you send command line input or pass file as input if they have implemented). If they have ensured that async is enabled when file size is large (basically some threshold on file size), ask them to set the threshold to some small value say 1 MB. Open the file to be written at the side or run `cat file.txt` at the Storage server periodically to see if content gets updated. - [25 Marks]
- Handling ACK : Ideally, supposed to send second ACK through NS. However, depending on how they are managing client sessions, it might have been challenging to reconnect with the client, they would have either not closed the client socket which initiated the request till entire async write was over or implemented a "client poll" where the client pings the NS to check if the async has been completed, and if they assumed that instead of NS, SS sends ACK to the client look at the assumptions they made. If found valid can award 50% marks. - [7.5 Marks].
- Handling partial writes - Repeat the test in point 1 and disconnect the SS midway. If client is notified award marks. - [10 Marks]
- Priority writes - A mechanism to support synchronous writing be it any sized file should be present. Test based on their implementation and if deemed correct award marks. - [7.5 Marks] 
