# BackupFilter
PoC filter driver to backup your documents - using cpp 

0. Create a backup directory if it does not exist yet (within the documents folder) 
1. Prior to initial modification of the file , driver reads the file contents from disk and writes them back to a dedicated backup directory
2. upon backing a file , the driver will send a message back to the client , and the client will print "document.docx was backed"
3. another cpp component - 'BackupRestore' will allow restoring of the whole directory or a specific file
4. any attempt to backup an existing file should overwrite the original backup - its the new restore point
5. as a bonus if we get there :: hide the backup directory  
