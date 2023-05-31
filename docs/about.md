
# About the Author

Ben Wirz is a Senior Electrical Engineer and Firmware Developer.  He is owner 
of [Element Products](https://www.elementinc.com), a product development and 
engineering company located near Denver, CO.   


# LPTASK History

Like most open source projects, LPTASK was born out of a need.  I had been 
working on porting a project to a different processor in early 2020.  The 
application had a large collection of infrequently called tasks which needed to 
be serviced. I wanted a cleaner and more maintainable method of organizing the 
tasks other than just checking task flags in the main loop. Checking task flags 
in main loop works just fine but isn't a really modular approach and tends to 
fall apart with a large number of tasks. To further complicate matters, the 
project had extremely aggressive power goals.  We were trying to achieve a 
2 year battery life from a CR2032 coin cell while still doing useful work.

After evaluating the usual RTOS's options I came to the conclusion that I 
could probably meet the power budget by using one of Tickless Modes but the 
RTOS's were  all pretty heavy requiring 10K+ of FLASH and 100+ bytes of RAM 
per task.  The RTOS's all seemed to be overkill for the project.  

Needing to move forward with the project, I decide the easiest solution was to 
write my own cooperative task scheduler using a statically defined tasks and a 
linked list.  Implementing a array of tasks would have been simpler than the 
linked list but it didn't achieve my goal being modular.  Supporting task 
definition inside the module where the task resided seemed a much cleaner 
approach to me.  I wrote and tested the first version in a couple of days and 
it worked great.  I went on to use the scheduler in a couple of other projects 
expanding its functionality as needed. 

In the summer of 2021 I realized that adding support for caching the next 
expired task had the potential to improve the scheduler's efficiency.  The 
scheduler was already searching for the next expiring task during the task 
service loop.  Caching the next expiring task saves the overhead of repeating 
the task search during the next task service loop in many cases.  Fewer 
processor cycles always equates with lower power so it seemed like an 
worthwhile feature to pursue.  I didn't have a immediate project need for the 
improvement but it turned out to be one of those ideas that I kept thinking 
about.  In the end, I had to implement it just to see if would work.    

Over the years, I've benefited from open source projects time and time again.  I 
felt like I was overdue to give something back to the community so I decided to 
share the project in Fall of 2022.  I spent the next a few months polishing the 
project, creating a test bench and documenting it during my free time.  I hope 
that you find it as useful as I have.  -Ben

