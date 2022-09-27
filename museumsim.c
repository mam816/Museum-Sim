#define VISITORS_PER_GUIDE 10
#define GUIDES_ALLOWED_INSIDE 2

struct shared_data {
	// Add any relevant synchronization constructs and shared state here.
	// For example:
	     pthread_mutex_t ticket_mutex;
		 pthread_cond_t visit; // need a condition variable for visitors
		 pthread_cond_t guide; //condition variable for guide
	     int tickets; //= (VISITORS_PER_GUIDE * GUIDES_ALLOWED_INSIDE); // max amount of allotted tickets/visitors at once
		 int guides;  //keep track of all guides in the museum
		 int visitors;  //keep track of all the visitors in the museum
		 int visitors_waiting; //visitors who have tickets who are waiting
		 int guides_waiting; //guides waiting for visitors
};

static struct shared_data shared;
//pthread_cond_init(&shared.cv, NULL);

/**
 * Set up the shared variables for your implementation.
 * 
 * `museum_init` will be called before any threads of the simulation
 * are spawned.
 */
void museum_init(int num_guides, int num_visitors)
{
	 pthread_mutex_init(&shared.ticket_mutex, NULL);
	 pthread_cond_init(&shared.visit, NULL);
	 pthread_cond_init(&shared.guide, NULL);
	 shared.tickets = MIN(VISITORS_PER_GUIDE * num_guides, num_visitors);
	 shared.guides = 0; // number of guides currently in museum
	 shared.visitors = 0; //number of visitors cuurently in the museum
	 shared.visitors_waiting = 0; // visitors waiting for guide
	 shared.guides_waiting = 0; // guides waiting for visitors
}


/**
 * Tear down the shared variables for your implementation.
 * 
 * `museum_destroy` will be called after all threads of the simulation
 * are done executing.
 */
void museum_destroy()
{
	 pthread_mutex_destroy(&shared.ticket_mutex); //clean up mutex
	 pthread_cond_destroy(&shared.visit); //clean up condition variable
	 pthread_cond_destroy(&shared.guide); //clean up condition variable
}


/**
 * Implements the visitor arrival, touring, and leaving sequence.
 
The visitor thread will indicate that it has arrived at 
the museum as soon as it has started.
However, the visitor should leave without touring 
if there are no tickets remaining.
After acquiring a ticket, the thread must get in line and **block** 
(i.e., wait) until there is a tour guide available to let the visitor in.
Unlike in a real museum, in the CS1550 museum, visitors who arrive may enter 
the museum in any order after they arrive, as long as there are no more than 
`GUIDES_ALLOWED_INSIDE * VISITORS_PER_GUIDE` inside the museum at any given time.
Once the visitor thread has entered the museum, it should indicate that it is 
touring by calling the `visitor_tours` mentioned below. Once a visitor
thread is done touring, it should indicate that it is leaving by calling 
`visitor_leaves`, and after that, it may do anything else necessary to leave. 
Each visitor's departure should **not** depend on any other visitor leaving,
or on a tour guide leaving. */

void visitor(int id)
{
	pthread_mutex_lock(&shared.ticket_mutex); //lock mutex
	
        // say that the visitor has arrived
	visitor_arrives(id);
	
	if(shared.tickets == 0)
	{
		//visitor leaves, no tickets available
		visitor_leaves(id);	
		pthread_mutex_unlock(&shared.ticket_mutex);
		return;
	}
   //if not all tickets are gone, give visitor ticket; down the 
   //the number of available tickets
   shared.tickets--;
   shared.visitors_waiting++; //now increase the number if visitors waiting to be admitted
   pthread_cond_broadcast(&shared.guide); // let guides know there is a visitor
   pthread_cond_wait(&shared.visit, &shared.ticket_mutex); //now make the visitor wait to be admitted by guide
   
  
    //while(shared.guides == 0)
	//{
	//	pthread_cond_wait(&shared.visit, &shared.ticket_mutex); // make condition variable wait until visitor can enter
	//shared.visitors_waiting++;
	//}
    
	

    pthread_mutex_unlock(&shared.ticket_mutex); // unlock mutex before the tour
    visitor_tours(id); //when in museum, call visitor_tours
    pthread_mutex_lock(&shared.ticket_mutex); //lock it back since tour finished
    //when tour is done, call visitor_leaves first, then up amount of available tickets, and down mutex i believe 
    visitor_leaves(id);

	
	
    shared.visitors--; // decrease number of current visitors
   
	
    pthread_cond_broadcast(&shared.guide); //now let the guide know when the visitor has left
    pthread_mutex_unlock(&shared.ticket_mutex); //unlock mutex

	 
}

/**
 * Implements the guide arrival, entering, admitting, and leaving sequence.
Like the visitor thread, the tour guide thread will indicate that it has arrived as soon as it has started. 
If there are already `GUIDES_ALLOWED_INSIDE` tour guides inside the museum, it must wait until all guides 
inside leave before entering. Once the tour guide is inside the museum, it should indicate that it has entered. 
Then, it must then begin waiting for and admitting visitors until it has either served a maximum of 
`VISITORS_PER_GUIDE` visitors, or there are no visitors waiting and no tickets remaining.
After the tour guide has admitted all the visitors it can, it must leave as soon as possible given 
the following constraints: it must wait for all visitors inside to leave, and for any other tour guide 
inside to finish before it can leave. Multiple tour guides inside must indicate they are leaving together 
before any new tour guides enter. The order in which multiple tour guides leave does not matter. 
If there are tour guides remaining but no visitors left to serve, the tour guides should enter and then immediately leave.
If another tour guide enters after a guide inside is done but before it leaves, both most wait until
 the one that entered is done serving visitors.
 */
void guide(int id)
{
	 pthread_mutex_lock(&shared.ticket_mutex); //lock mutex
	 guide_arrives(id); // guide arrives
         int visitInside = 0; //this is the number of people inside so the guides can keep count to know when they can leave
	 while(shared.guides == GUIDES_ALLOWED_INSIDE)
	 {
		pthread_cond_wait(&shared.guide, &shared.ticket_mutex); // make guide wait until other guides leaves
	 }
	 
	 while(shared.guides_waiting > 0) // if there are guides waiting to enter but there is already some inside, it has to wait to enter
	 {
        	 pthread_cond_wait(&shared.guide, &shared.ticket_mutex);
	 }
	 guide_enters(id); //guide goes into museum
	 shared.guides++; // increase number of guides inside museum

	 
	 while(shared.tickets != 0 && shared.visitors_waiting == 0)
	 {
		 pthread_cond_wait(&shared.guide, &shared.ticket_mutex); // make guide wait if there are still some tickets available and no one waiting
	 }
	
	if(shared.tickets == 0 && shared.visitors_waiting == 0)
	 {
           shared.guides--; // decrease number of guides inside
	   guide_leaves(id); //guide can leave
	   pthread_mutex_unlock(&shared.ticket_mutex);
	   return;
	 }


	 while(VISITORS_PER_GUIDE > visitInside)
	 {
		 if(shared.tickets > 0) // if the tickets is greater than 0; tickets left to give out
		 {
		   pthread_cond_wait(&shared.guide, &shared.ticket_mutex); //make the guide wait for more visitors
		 }
		 if(shared.visitors_waiting > 0) //if there are visitors waiting to be let in
		 {
		   pthread_cond_signal(&shared.visit); //send a signal to wake up waiting visitors
		   guide_admits(id); //admit them
		   shared.visitors++; //increase number of visitors inside museum
		   visitInside++; //same thing as above, but for guides count instead of shared; sake of loop
		   shared.visitors_waiting--; //decrease number of waiting visitors as they are admited
		 }
		else
		 {	
       		   //break;  //if no one is waitng or if no one is there, break out of loop so guide can leave towards the bottom
		   shared.guides_waiting++;
		   while(shared.visitors > 0) 
	   		{
			  pthread_cond_wait(&shared.guide, &shared.ticket_mutex);
	                }
	           while(shared.guides_waiting < shared.guides)
	  		{
			  pthread_cond_wait(&shared.guide, &shared.ticket_mutex); 
	                }
		   pthread_cond_broadcast(&shared.guide);
	 	   guide_leaves(id); //guide leaves
	 	   shared.guides--; // decrease number of guides
	 	   shared.guides_waiting--; //decrease number of guides waiting
	 	   pthread_mutex_unlock(&shared.ticket_mutex); //unlock mutex 
		   return;
		 }
	 } 
    
	shared.guides_waiting++; // increase guides waiting since there are guides inside the museum now and have to wait on each other in order to leave or enter if already full

        while(shared.visitors > 0) 
	    {
		pthread_cond_wait(&shared.guide, &shared.ticket_mutex);
	    }
	while(shared.guides_waiting < shared.guides)
	    {
		pthread_cond_wait(&shared.guide, &shared.ticket_mutex); 
	    }
	 pthread_cond_broadcast(&shared.guide);
	 guide_leaves(id); //guide leaves
	 shared.guides--; // decrease number of guides
	 shared.guides_waiting--; //decrease number of guides waiting
	 pthread_mutex_unlock(&shared.ticket_mutex); //unlock mutex
}
