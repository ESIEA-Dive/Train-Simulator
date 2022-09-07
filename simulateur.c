#include <stdio.h> 
#include <stdlib.h> // For exit() 
#include <string.h>
#include <unistd.h>
#include <time.h>

#include<signal.h>
#include<string.h>
#include<termios.h>
#include<unistd.h>
#include<fcntl.h>
#include<pthread.h>

#define gotoxy(x,y) printf("\033[%d;%dH", (x), (y))



typedef struct plan{

    int rows;
    int cols;
    char** matrix;
}plan;

typedef struct train{
    int dir;  /*N => Nord, S => Sud, E => EST, O => OUEST*/
    int posx;         /*Position courante x de la tête du train*/
    int posy;         /*Position courante y de la tête du train*/
    plan* p; /*Contient le train customisé, il faut choisirla bonne taille de votre tableau*/
    plan* po;
    char etat;        /*État du train => dehors, entrant, stationné,sortant, sorti*/
    int portes ;      /*Portes ouvertes ou fermées*//*Vous pouvez bien-sur rajouter d’autres variables si nécessaire*/
    int speed;
}train;

typedef struct voyageur{

        int posx;
        int posy;
        int type; //0 montant 1 descendant

}voyageur;

typedef struct list_voyageurs{

    voyageur* v;
    struct list_voyageurs* next;

}list_voyageurs;

typedef struct station{

        plan* p;
        int posx_quai;
        int stop_pos;
        int end_quai;

        int posx_quai_;
        int stop_pos_;
        int end_quai_;

        int x_voyageur_max_quai;
        int x_voyageur_min_quai;
        int x_voyageur_max_quai_;
        int x_voyageur_min_quai_;
        int y_voyageur_min;
        int y_voyageur_max;

        list_voyageurs* v_quai;
        list_voyageurs* v_quai_;
    
        int etat_quai;
        int etat_quai_;
        int sec_attente;
        int sec_attente_;
        int heure_pointe;


}station;

pthread_mutex_t lock;
voyageur* creat_voyageur(int posx, int posy, int type);
voyageur* generate_rand_voyageur_(station* st, int type, int dir);


int find_voyageur_in_list(list_voyageurs* list, int posx, int posy){

    list_voyageurs* tmp= list;
    while(tmp){
        if(tmp->v->posx==posx && tmp->v->posy==posy)
            return 1;
        tmp= tmp->next;
    }
    return 0;
}

voyageur* generate_rand_voyageur_(station* st, int type, int dir){


    int posx, posy;

    if(!dir){
        if(!type){
            posx= st->x_voyageur_min_quai;
        }
        else{
            posx= st->x_voyageur_max_quai;
        }
    }
    else{
        if(!type){
            posx= st->x_voyageur_max_quai_-1;
        }
        else{
            posx= st->x_voyageur_min_quai_;
        }
    }
    posy= random()%(st->y_voyageur_max-st->y_voyageur_min+1) + st->y_voyageur_min;

    list_voyageurs* list;
    if(!dir)
        list= st->v_quai;
    else
        list= st->v_quai_;

    if(!find_voyageur_in_list(list, posx, posy))
        return(creat_voyageur(posx, posy, type));

    return NULL;

}

void add_voyageur_to_station(station* st, int type, int dir){


    voyageur* v= generate_rand_voyageur_(st, type, dir);

    if(v){
        list_voyageurs* l= (list_voyageurs*)malloc(sizeof(list_voyageurs));
        l->v= v;
        if(!dir){
            l->next= st->v_quai;
            st->v_quai= l;
        }
        else{

            l->next= st->v_quai_;
            st->v_quai_= l;
        }
    }
}




void free_list_voyageurs(list_voyageurs* l){

    if(l){
        free_list_voyageurs(l->next);
	if(l->v)
            free(l->v);
        free(l);    
    }
}



void print_plan(plan* p, int posx, int posy, int shift);
void free_plan(plan* p);

int nb_rows_in_file(char* filename){

    char *line_buf = NULL;
    size_t line_buf_size = 0;
    int line_count = 0;
    ssize_t line_size;
    int max_size=0;
   
    FILE *fp = fopen(filename, "r");
    if (!fp)
        fprintf(stderr, "Error opening file '%s'\n", "filename");

    /* Get the first line of the file. */
    line_size = getline(&line_buf, &line_buf_size, fp);
    /* Loop through until we are done with the file. */
    while (line_size >= 0){
        /* Increment our line count */
        line_count++;
        /* Show the line details */
        line_size = getline(&line_buf, &line_buf_size, fp);
    }

    fclose(fp);

    return line_count;
}


int nb_max_cols_in_file(char* filename){

    char *line_buf = NULL;
    size_t line_buf_size = 0;
    int max_cols = 0;
    ssize_t line_size;
   
    FILE *fp = fopen(filename, "r");
    if (!fp)
        fprintf(stderr, "Error opening file '%s'\n", "filename");

    /* Get the first line of the file. */
    line_size = getline(&line_buf, &line_buf_size, fp);

    /* Loop through until we are done with the file. */
    while (line_size >= 0){

        if(strlen(line_buf)>max_cols){
	        max_cols= strlen(line_buf);
        }
        /* Show the line details */
        line_size = getline(&line_buf, &line_buf_size, fp);
    }
    fclose(fp);
        
    return max_cols;
}



plan* load_plan(char* filename){

    plan* p= (plan*)malloc(sizeof(plan));
    
    /* Open the file for reading */
    char *line_buf = NULL;
    size_t line_buf_size = 0;
    ssize_t line_size;
    int num_line=0;

    p->rows= nb_rows_in_file(filename);
    p->cols= nb_max_cols_in_file(filename);
    p->matrix= (char**)malloc(sizeof(char*)*p->rows);

    for(int i=0; i<p->rows; i++)
        p->matrix[i]= (char*)malloc(sizeof(char)*(p->cols+1));

    FILE *fp = fopen(filename, "r");
    if (!fp)
        fprintf(stderr, "Error opening file '%s'\n", "filename");

    line_size = getline(&line_buf, &line_buf_size, fp);
    /* Loop through until we are done with the file. */
    while (line_size >= 0){
        
        strcpy(p->matrix[num_line], line_buf);
        line_size = getline(&line_buf, &line_buf_size, fp);
        num_line++;
    }
    fclose(fp);

    return p;
}



char* load_file(char* filename);

train* creat_train(char* filename, char* filename_po, int posx, int posy, int dir){

    train* t= (train*)malloc(sizeof(train));
    t->p= load_plan(filename);
    t->po= load_plan(filename_po);
    t->posx= posx;
    t->posy= posy;
    t->dir= dir;
    t->speed= 30000;

    return t;
}

void free_train(train* t){

    free_plan(t->p);
    free(t);
}

station* creat_station(char* filename, int stop_pos, int end_quai){

    station* st= (station*)malloc(sizeof(station));
    st->p= load_plan(filename);
    st->stop_pos= stop_pos;
    st->end_quai= end_quai;
    st->posx_quai= 16;
    st->end_quai_= end_quai;
    st->stop_pos_= 17;
    st->posx_quai_= 25;

    st->x_voyageur_max_quai= st->posx_quai-1;
    st->x_voyageur_min_quai= 7;

    st->x_voyageur_min_quai_= st->posx_quai_+5;
    st->x_voyageur_max_quai_= 40;

    st->y_voyageur_min= st->stop_pos_+2;
    st->y_voyageur_max= st->stop_pos-6;

    st->etat_quai= 0;
    st->etat_quai_= 0;

    st->sec_attente= 60;
    st->sec_attente= 60;

    st->heure_pointe=0;
    st->v_quai= NULL;
    st->v_quai_=NULL; 

    return st;
}

void free_station(station* st){
    
    free_plan(st->p);
    free(st);
    free_list_voyageurs(st->v_quai);
    free_list_voyageurs(st->v_quai_);

}

void remove_voyageurs_from_station(station* st, int dir){

    if(!dir){
        free_list_voyageurs(st->v_quai);
        st->v_quai= NULL;
    }
    else
        free_list_voyageurs(st->v_quai_);
        st->v_quai_= NULL;
}


voyageur* creat_voyageur(int posx, int posy, int type){

    voyageur* v= (voyageur*)malloc(sizeof(voyageur));

    v->posx= posx;
    v->posy= posy;
    v->type= type;
}



void print_voyageur(voyageur* v){

    gotoxy(v->posx, v->posy);
    if(!v->type)
        printf("X\n");
    else
        printf("O\n");
}

void print_list_voyageur(list_voyageurs* l){

    list_voyageurs* tmp= l;
    while(tmp){

    pthread_mutex_lock(&lock);
        print_voyageur(tmp->v);
    pthread_mutex_unlock(&lock);
        tmp= tmp->next;
    }

}

void update_quai(plan* s, int num_quai){
    

    pthread_mutex_lock(&lock);

    if(!num_quai){
        gotoxy(16, 0);
        for(int i=16; i<22;i++) 
            printf("%s", s->matrix[i]);
    }
    else{
        gotoxy(22, 0);
        for(int i=22; i<30;i++) 
            printf("%s", s->matrix[i]);
    }

    pthread_mutex_unlock(&lock);
}

void update_espace_voyageur(station* st, int num_quai){


    pthread_mutex_lock(&lock);

    if(!num_quai){
        gotoxy(st->x_voyageur_min_quai, 0);
        for(int i=st->x_voyageur_min_quai; i<st->x_voyageur_max_quai; i++)
            printf("%s", st->p->matrix[i]);
    }
    else{
        gotoxy(st->x_voyageur_min_quai_, 0);
        for(int i=st->x_voyageur_min_quai_; i<=st->x_voyageur_max_quai_+2; i++)
            printf("%s", st->p->matrix[i]);
    }

    pthread_mutex_unlock(&lock);

}


void move_voyageurs(station* st, int dir){


    if(!dir){

        list_voyageurs* l= st->v_quai;

        while(l){
            if(!l->v->type){
                if(st->etat_quai)
                    l->v->posx+= (rand()%2 +1);
            }
            else{
                l->v->posx-= 1;
            }
            l= l->next;
        }
    }
    else{

        list_voyageurs* l= st->v_quai_;

        while(l){
            if(!l->v->type){
                 if(st->etat_quai_)
                    l->v->posx-= rand()%2+1;
            }
            else{
                l->v->posx= (l->v->posx+1);
            }
            l= l->next;

        }
    }



}



list_voyageurs* remove_voyageur(list_voyageurs* l, int x_min, int x_max){

    list_voyageurs* tmp= l;
    if(l){

        if(l->v->posx< x_min || l->v->posx> x_max){
            tmp= l->next;
            free(l->v);
            free(l);
            return(remove_voyageur(tmp, x_min, x_max));
        }
        else{
            l->next= remove_voyageur(l->next, x_min, x_max);
            return l;
        }
    }
    return NULL;

}



void update_voyageurs(station* st, int dir){

    if(!dir){
        st->v_quai= remove_voyageur(st->v_quai, st->x_voyageur_min_quai, st->x_voyageur_max_quai);
    }
    else{
        st->v_quai_= remove_voyageur(st->v_quai_, st->x_voyageur_min_quai_, st->x_voyageur_max_quai_);
    }

}

                                       
char* load_file(char* filename){

    FILE *fp;
    long lSize;
    char *buffer;

    fp = fopen ( filename , "rb" );
    if( !fp ) perror(filename),exit(1);
    fseek( fp , 0L , SEEK_END);
    lSize = ftell( fp );
    rewind( fp );

    /* allocate memory for entire content */
    buffer = calloc( 1, lSize+1 );
    if( !buffer ) fclose(fp),fputs("memory alloc fails",stderr),exit(1);

    /* copy the file into the buffer */
    if( 1!=fread( buffer , lSize, 1 , fp) )
        fclose(fp),free(buffer),fputs("entire read fails",stderr),exit(1);

    /* do your work here, buffer is a string contains the whole text */
    fclose(fp);

    return buffer;
}

  

void print_plan(plan* p, int posx, int posy, int shift){


    pthread_mutex_lock(&lock);

    for(int i=0; i<p->rows; i++){
        gotoxy(posx+i, posy);
        if(shift>=0){
            printf("%s", (p->matrix[i]+shift));
        }
        else{
            
            for(int j=0; j<strlen(p->matrix[i])+shift; j++)
                printf("%c", p->matrix[i][j]);
            
            printf("\n"); 
        }
    }
    
    pthread_mutex_unlock(&lock);
}

void free_plan(plan* p){

    for(int i=0;i<p->rows; i++)
        free(p->matrix[i]);
    free(p->matrix);

}

int stop_simulation;

void print_temps_attente(station* st, int dir, int min, int sec);
char key_pressed();

char print_entree_train_(station* st , train* t){
    char c;
    if(!t->dir){
        int i;
        for(i=0; i<st->stop_pos; i++){
            update_quai(st->p, t->dir);
            if(i<t->p->cols){
                print_plan(t->p, st->posx_quai, 0, t->p->cols-i);
            }
            else{
                print_plan(t->p, st->posx_quai, i-t->p->cols, 0);
            }
            print_temps_attente(st, t->dir, 0, 0);
            c= key_pressed();
            if(c=='s' || stop_simulation)
                return c;
            usleep(t->speed);
        }
        t->posy= st->stop_pos;
        print_plan(t->po, st->posx_quai, i-t->p->cols-1, 0);
        st->etat_quai= 1;

    }
    else{
        int i;
        for(i=st->end_quai_-1; i>st->stop_pos_; i--){
            update_quai(st->p, t->dir);
            if(st->end_quai_-i<t->p->cols)
                print_plan(t->p, st->posx_quai_, i-2, (st->end_quai_-i)-t->p->cols);
            else{
                print_plan(t->p, st->posx_quai_, i-2, 0);
            }
            print_temps_attente(st, t->dir, 0, 0);
            c= key_pressed();
            if(c=='s' || stop_simulation)
                return c;
            usleep(t->speed);
        }
        t->posy= st->stop_pos_;
        print_plan(t->po, st->posx_quai_, i-1, 0);
        st->etat_quai_= 1;
    }
    return c;
}

char printf_sortie_train_(station* st, train* t);


char entree_sortie_train_trafic(station* st, train* t, int with_voyageur){

    

    char c;
    c= print_entree_train_(st , t);
    if(c=='s' || stop_simulation)
        return c;
    sleep(1);
    int nb= (st->heure_pointe)?8:3;
    for(int i=0; i<10; i++){

        if(!(i%2) && i<6 && with_voyageur){
            for(int j=0; j<nb; j++){
                add_voyageur_to_station(st, 1, t->dir);
            }
        }
        else if(i<6 && with_voyageur){
                for(int j=0; j<nb; j++){
                    add_voyageur_to_station(st, 0, t->dir);
                }
        }
        if(with_voyageur){
            update_espace_voyageur(st, t->dir);
            update_voyageurs(st, t->dir);
        }
        if(!t->dir){
            
            print_list_voyageur(st->v_quai);
            print_temps_attente(st, t->dir, st->sec_attente/60, st->sec_attente%60);
        }
        else{
            
            print_list_voyageur(st->v_quai_);
            print_temps_attente(st, t->dir, st->sec_attente_/60, st->sec_attente_%60);
        }
        c=key_pressed();
        if(c=='s' || stop_simulation)
            return c;
        sleep(1);
        if(i+1<10 && with_voyageur)
        move_voyageurs(st, t->dir);
 
    }
    c= printf_sortie_train_(st, t);
    return c;
    

}


void trafic_train_simulation(station* st, train* t, int heure_pointe, int with_voyageur){

    int j=0;
    if(!t->dir)
        st->sec_attente= (heure_pointe)? 30:60;
    else
        st->sec_attente_= (heure_pointe)? 20:50;
    st->heure_pointe= heure_pointe;
    int sec= (!t->dir)? st->sec_attente: st->sec_attente_;
    char c;

    while(1){

        for(int i=0; i<sec; i++){
            if(with_voyageur){
                update_espace_voyageur(st, t->dir);
                move_voyageurs(st, t->dir); 
                update_voyageurs(st, t->dir);
            }
            if(!t->dir){
                print_list_voyageur(st->v_quai);
                print_temps_attente(st, t->dir, (st->sec_attente-i)/60, (st->sec_attente-i)%60);
            }
            else{
                
                print_list_voyageur(st->v_quai_);
                print_temps_attente(st, t->dir, (st->sec_attente_-i)/60, (st->sec_attente_-i)%60);
            }
            c= key_pressed();
            if(c=='s' || stop_simulation){
                stop_simulation= 1;    
                return;
            }
            sleep(1);
        }
        c= entree_sortie_train_trafic(st, t, with_voyageur);
        if(c=='s' || stop_simulation){
            stop_simulation= 1;
            return;
        }
        
    }

}


void print_temps_attente(station* st, int dir, int min, int sec){

    pthread_mutex_lock(&lock);

    if(dir){
        gotoxy(st->x_voyageur_max_quai_, st->y_voyageur_max+8);
    }
    else{
        gotoxy(st->x_voyageur_min_quai, st->y_voyageur_min-12);
    }

    if(min<10)
        printf("0");
    printf("%d:", min);
    if(sec<10)
        printf("0");
    printf("%d\n", sec);
    
    pthread_mutex_unlock(&lock);

}

char printf_sortie_train_(station* st, train* t){

    char c;
    if(!t->dir){
        st->etat_quai= 0;
        int reste_quai_dist= st->end_quai-t->posy;
        float var=10;
        for(int i= t->posy+1; i<t->posy+t->p->cols+reste_quai_dist-1; i++){

            update_quai(st->p, t->dir);
            if(i>st->end_quai){
                print_plan(t->p, st->posx_quai, i-t->p->cols-1, st->end_quai-i);
            }
            else{
                print_plan(t->p, st->posx_quai, i-t->p->cols-1, 0);
            }

            update_espace_voyageur(st, t->dir);
            move_voyageurs(st, t->dir); 
            update_voyageurs(st, t->dir);
            print_list_voyageur(st->v_quai);

            print_temps_attente(st, t->dir, st->sec_attente/60, st->sec_attente%60);
            c= key_pressed();
            if(c=='s' || stop_simulation)
                return c;
            usleep(t->speed*var);
            var= ((var-0.15)<=1)? 1:(var-0.15);
        }
        update_quai(st->p, t->dir);
    }
    else{
        st->etat_quai_= 0;
        float var= 10;
        for(int i=2; i<t->posy+t->p->cols-1; i++){
            update_quai(st->p, t->dir);
            if(i<t->posy){

                print_plan(t->p, st->posx_quai_, t->posy-i, 0);
            }
            else{

                print_plan(t->p, st->posx_quai_, 0, i-t->posy+1);    
            }

            update_espace_voyageur(st, t->dir);
            move_voyageurs(st, t->dir); 
            update_voyageurs(st, t->dir);
            print_list_voyageur(st->v_quai_);

            print_temps_attente(st, t->dir, st->sec_attente_/60, st->sec_attente_%60);
            c= key_pressed();
            if(c=='s' || stop_simulation)
                return c;
            usleep(t->speed*var);
            var= ((var-0.15)<=1)? 1:(var-0.15);

        }
        update_quai(st->p, t->dir);        
    }
    return c;

}


char key_pressed(){

    struct termios oldterm, newterm;
    int oldfd; char c, result = 0;
    tcgetattr (STDIN_FILENO, &oldterm);
    newterm = oldterm; newterm.c_lflag &= ~(ICANON | ECHO);
    tcsetattr (STDIN_FILENO, TCSANOW, &newterm);
    oldfd = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl (STDIN_FILENO, F_SETFL, oldfd | O_NONBLOCK);
    c = getchar();
    tcsetattr (STDIN_FILENO, TCSANOW, &oldterm);
    fcntl (STDIN_FILENO, F_SETFL, oldfd);
    if (c != EOF) {
	    ungetc(c, stdin); 
	    result = getchar();
	}
    return result;
}


void clearScreen()
{
  const char *CLEAR_SCREEN_ANSI = "\e[1;1H\e[2J";
  write(STDOUT_FILENO, CLEAR_SCREEN_ANSI, 12);
}

typedef struct thread_params{
    station* st;
    train* t;
    int heure_pointe;
    int with_voyageur;
}thread_params;

void* thread_function(void* params){

    thread_params* params_cp= (thread_params*)params;
    if(params_cp){
        trafic_train_simulation(params_cp->st, params_cp->t, 
        params_cp->heure_pointe, params_cp->with_voyageur);
    }
}


int main() 
{ 
    /* Intializes random number generator */  
    time_t tm;
    srand((unsigned) time(&tm));
    char* Modes[4]={"Trafic Normal Sans Voyageur", "Heure de Pointe Sans voyageur", "Trafic normal + voyageur", "Heure de pointe + voyageur"};
    char* plan_file= "SIMULATEUR_METRO_cp.txt";
    train* t= creat_train("train.txt", "train_po.txt", -1, -1, 0);   
    station* st= creat_station("SIMULATEUR_METRO_cp_.txt", 125, 140);
    train* t1= creat_train("train_r.txt", "train_r_po.txt", -1, -1, 1);   
    pthread_t server_thread_quai, server_thread_quai_;
    int server_thread_status_quai, server_thread_status_quai_;
    char c;
    

    thread_params* params_quai= (thread_params*)malloc(sizeof(thread_params));
    thread_params* params_quai_= (thread_params*)malloc(sizeof(thread_params));

    params_quai->st= st; params_quai_->st= st;
    params_quai->t= t;
    params_quai_->t= t1;
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }    

    while(1){
        clearScreen();
        gotoxy(0, 0);   
        printf("1) Train circulent sans voyageur en trafic normal\n");
        printf("2) Train circulent sans voyageur en heure de pointe\n");
        printf("3) Train + voyageurs en trafic normal\n");
        printf("4) Train + voyageurs en heure de pointe\n");
        printf("5) Quiter\n");
        while(1){        
            c=key_pressed();
            if(c=='1' || c=='2' || c=='3' || c=='4' || c=='5')
                break;
            sleep(1);
        }
        
        if(c=='1'){
            params_quai->with_voyageur= 0;
            params_quai->heure_pointe= 0;
            params_quai_->with_voyageur= 0;
            params_quai_->heure_pointe= 0;

        }
        else if(c=='2'){
            params_quai->with_voyageur= 0;
            params_quai->heure_pointe= 1;
            params_quai_->with_voyageur= 0;
            params_quai_->heure_pointe= 1;

        }
        else if(c=='3'){
            params_quai->with_voyageur= 1;
            params_quai->heure_pointe= 0;
            params_quai_->with_voyageur= 1;
            params_quai_->heure_pointe= 0;

        }
        else if(c=='4'){
            params_quai->with_voyageur= 1;
            params_quai->heure_pointe= 1;
            params_quai_->with_voyageur= 1;
            params_quai_->heure_pointe= 1;

        }
        else if(c=='5'){
            break;
        }
        
        remove_voyageurs_from_station(st, 0);
        remove_voyageurs_from_station(st, 1);
        print_plan(st->p, 0, 0, 0);
        printf("                                    \
                Mode: %s\n", Modes[c-'1']);
        printf("                                \
                Appuyer sur 's' pour arreter la simulation\n");
        stop_simulation=0;
    
        server_thread_status_quai = pthread_create(&server_thread_quai, NULL, thread_function, params_quai);
        server_thread_status_quai_ = pthread_create(&server_thread_quai_, NULL, thread_function, params_quai_);

        pthread_join (server_thread_quai, NULL);
        pthread_join (server_thread_quai_, NULL);

        //trafic_train_simulation(st, t, heure_pointe, with_voyageur);


    }
    
    pthread_mutex_destroy(&lock);
    gotoxy(70, 0);
    system("reset");

    free_train(t);
    free_train(t1);
    free_station(st);
    free(params_quai);
    free(params_quai_);

    return 0; 
}
