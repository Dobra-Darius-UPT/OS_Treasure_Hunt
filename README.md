# OS_Treasure_Hunt
Build a C program in the UNIX environment simulating a treasure hunt game system that allows users to create, manage, and participate in digital treasure hunts. The system will use files to store treasure clues and locations, manage game state through processes and signals.

 ------ post at least once a week, and mandatorry twice a week ------

 I. FIRST ASSIGNMENT
    - Create the foundation for storing and managing treasure hunt data using file operations.

    WEEK I:
      - implemented the functions:
         - get_treasure_input():
            - will ask for the specific data of a treasure
         - check_hunt():
            - we need to check if the hunt exists or not
         - filter_hunt(): (BAD NAME)
            - creates a new directory for a hunt if the hunt does not    exists
   
   WEEk II: 
      - added the full functionality of the treasure manager
      - modifyed some of the older functions in order to achive a better structure

   WEEK III:
      - SPRING BREAK

   WEEK IV:
      TREASURE_HUB
      - created the treasure_hub with dependence need of the treasure_monitor in order to communicate via signals in order to complete the task.
      - for the treasure_hub i managed to display all the functions needed in terminal in order to be available for a user and also the signals checkup
      for all the cases(via process ID "pid")
      TREASURE_MANAGER 