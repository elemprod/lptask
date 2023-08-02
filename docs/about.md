
# About the Author

Ben Wirz is a Senior Electrical and Firmware Engineer.  He is owner of 
[Element Products](https://www.elementinc.com), a product development and 
contract engineering company located outside of Denver, CO.  

# LPTASK History

Like many open-source projects, LPTASK was born out of a need.  I was working
on developing an embedded application which contained a large collection of 
infrequently serviced tasks.  The project had an extremely aggressive power 
goal.  We were targeting a 2-year battery life from a CR2032 coin cell while 
still performing useful work.  The reductions in power consumption 
made by microcontroller vendors in the past decade made the power goals 
achievable. 

It is a common practice to service expired or pending tasks from inside the main 
loop.  This approach work well for small applications but can become 
burdensome with a large number of tasks.  I wanted a cleaner and more modular 
way of organizing the tasks for the project.  After evaluating the usual RTOS's 
options, I came to the conclusion that while I could probably meet the power 
budget by using an RTOS tickless mode, the RTOS implementation still felt
pretty heavy requiring 10K+ of FLASH and 100+ bytes of RAM per task.  

In the end, I decided that the easiest solution was to write my own cooperative 
task scheduler using statically defined tasks and a linked list task que.  
Implementing an array of tasks might have been simpler than the linked list but 
it wouldn't achieve my goal being modular.  Supporting task definition inside 
the module where the task was utilized seemed to be a cleaner approach to 
me.  I wrote and tested the first version of the scheduler in a few days and it 
worked well for the project.  I went on to use the scheduler in a couple of 
other projects, expanding the functionality as needed. 

I eventually realized that adding support for caching the next expired 
task had the potential to improve the scheduler's efficiency.  The scheduler 
had been calculating each task's expiration time during the task service loop.  
If the next expiring task were saved, the scheduler would only need to check it 
for expiration.  This would save the overhead of repeating the expiration check 
on each task in cases where the task que had not been modified.  Fewer 
processor cycles nearly always equates to lower power so it seemed like an 
worthwhile feature to pursue.

I have benefited from open source projects numerous times.  I felt like I was 
overdue to give something back to the community so I  decided to share the 
project.  I hope that you find it as useful as I have.  -Ben

