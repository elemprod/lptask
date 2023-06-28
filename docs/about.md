
# About the Author

Ben Wirz is a Senior Electrical and Firmware Engineer.  He is owner of 
[Element Products](https://www.elementinc.com), a product development and 
contract engineering company located outside of Denver, CO.  

# LPTASK History

Like most open source projects, LPTASK was born out of a need.  I had been 
working on porting a project to a new processor in 2020.  The 
application had a large collection of infrequently called tasks which needed to 
be serviced periodically. I wanted a cleaner and more maintainable method of 
organizing the tasks other than just checking task flags in the main loop. 
Checking flags works just fine for small applications but tends to fall apart 
with a large number of tasks.  It's also not a modular approach. To further 
complicate matters, the project had extremely aggressive power goals.  I was 
trying to achieve a 2 year battery life from a CR2032 coin cell while still 
performing useful work.

After evaluating the usual RTOS's options I came to the conclusion that I 
could probably meet the power budget by using a tickless mode but the 
RTOS's were all pretty heavy requiring 10K+ of FLASH and 100+ bytes of RAM 
per task.  The RTOS approach felt like overkill for the project.  

In the end, I decided the easiest solution was to write my own cooperative task 
scheduler using a statically defined tasks and a linked list.  Implementing an 
array of tasks would have been simpler than the linked list but it wouldn't 
achieve my goal being modular.  Supporting task definition inside the module 
where the task was utilized seemed a much cleaner approach to me.  I wrote and 
tested the first version of the scheduler in a couple of days and it worked well 
for the project.  I went on to use the scheduler in a couple of other projects 
expanding its functionality as needed. 

At a later point, I realized that adding support for caching the next expired 
task had the potential to improve the scheduler's efficiency.  The scheduler 
was already searching for the next expiring task during the task 
service loop.  Caching it for future use would save the overhead of repeating 
the search in cases where the task que had not been not modified.  Fewer 
processor cycles always equates to lower power so it seemed like an 
worthwhile feature to pursue. 

Over the years, I've benefited from open source projects time and time again.  
I felt like I was overdue to give something back to the community so I  
decided to share the project.  I hope that you find it as useful as I 
have.  -Ben

