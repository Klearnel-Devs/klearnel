/*-------------------------------------------------------------------------*/
/**
   \file    quarantine.h
   \author  Copyright (C) 2014, 2015 Klearnel-Devs 
   \brief   Quarantine header

   This file contains all structure and includes needed by 
   the quarantine routines
*/
/*--------------------------------------------------------------------------*/
#ifndef _KLEARNEL_QUARANTINE_H
#define _KLEARNEL_QUARANTINE_H

/*---------------------------------------------------------------------------
                                Definitions
 ---------------------------------------------------------------------------*/
/**
  \brief The folder where elements in Quarantine are stored
 */
#define QR_STOCK 	WORK_DIR "/qr_stock"
/**
  \brief Quarantine database file
 */
#define QR_DB 		BASE_DIR "/qr.db"
/**
  \brief Temporary Quarantine folder
 */
#define QR_TMP      TMP_DIR "/quarantine"
/**
  \brief Quarantine Unix Domain Socket file
 */
#define QR_SOCK 	TMP_DIR "/kl-qr-sck"
/**
  \brief Default restore folder

  If the original location of a file to restore doesn't exist
  anymore, the file is restored to this location
 */
#define RES_DEF     "/tmp/klearnel"
/*-------------------------------------------------------------------------*/
/**
  \brief The "add to Quarantine" signal
 */                                
#define QR_ADD 		    1
/**
  \brief The "remove from Quarantine" signal
 */
#define QR_RM 		    2
/**
  \brief The "restore from Quarantine" signal
 */
#define QR_REST		    3
/**
  \brief The "get list from Quarantine" signal
 */
#define QR_LIST 	    4
/**
  \brief The "get information of one item in Quarantine" signal
 */
#define QR_INFO		    5
/**
  \brief The "remove all items from Quarantine" signal
 */
#define QR_RM_ALL       6
/**
  \brief The "restore all items from Quarantine" signal
 */
#define QR_REST_ALL     7
/**
  \brief Used by QR_RM_ALL & QR_REST_ALL signals to reload the list
 */
#define QR_LIST_RECALL  8

/*---------------------------------------------------------------------------
                                New types
 ---------------------------------------------------------------------------*/

/* Structure of file into quarantine */
/*-------------------------------------------------------------------------*/
/**
  \brief    QrData Structure

  Structure containing all relevant information pertaining
  to a file in Quarantine

 */
/*-------------------------------------------------------------------------*/
struct qr_file {
	char 		f_name[256];		//!< Filename of the fil in qr
	char		o_path[PATH_MAX]; 	//!< Old path to restore
	struct stat	o_ino; 			//!< Inode info before move into quarantine
	time_t 		d_begin; 		//!< Date at which file has been moved to quarantine
	time_t 		d_expire; 		//!< Date at which file will be dropped out (optional)
};

/**
 \brief Define the QrData type as we use it in quarantine.c
*/
typedef struct qr_file QrData;

struct QrListNode;

/*-------------------------------------------------------------------------*/
/**
  \brief    Structure containing QrData 

  Contains a QrData struct and pointers to both the next
  QrListNode and the previous QrListNode in the doubly
  linked list

 */
/*-------------------------------------------------------------------------*/
typedef struct QrListNode {
    struct QrListNode *next; //!< Pointer to next node in list
    struct QrListNode *prev; //!< Pointer to previous node in list
    QrData data;             //!< This nodes data payload
} QrListNode;

/*-------------------------------------------------------------------------*/
/**
  \brief    QrList

  The brain of the doubly linked list. Contains a count
  of the number of items and pointers to both the first
  and last QrListNodes in the list

 */
/*-------------------------------------------------------------------------*/
typedef struct QrList {
    int count;          //!< The count of items in the doubly linked list
    QrListNode *first;  //!< Pointer to first node in list
    QrListNode *last;   //!< Pointer to last node in list
} QrList;

/**
  \brief Macro to traverse the list of the Quarantine
*/
#define LIST_FOREACH(L, S, M, V) QrListNode *_node = NULL;\
    QrListNode *V = NULL;\
    for(V = _node = L->S; _node != NULL; V = _node = _node->M)

#define TMP_LIST_FOREACH(L, S, M, V) QrListNode *_node = NULL;\
    QrListNode *V = NULL;\
    for(V = _node = (*L)->S; _node != NULL; V = _node = _node->M)

/*---------------------------------------------------------------------------
                                Prototypes
 ---------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/**
  \brief    Initialize all requirements for Quarantine
  \return   void

  
 */
/*--------------------------------------------------------------------------*/
int is_empty();

void init_qr();
/*-------------------------------------------------------------------------*/
/**
  \brief        Loads Quarantine into temporary file
  \param        list    The Quarantine list
  \param        fd      The custom file descriptor
  \return       void

  Used principally to allow users to list QR files
 */
/*--------------------------------------------------------------------------*/
void load_tmp_qr(QrList** list, int fd);
/*-------------------------------------------------------------------------*/
/**
  \brief        Load quarantine with content of QR_DB  
  \param        list    The Quarantine list
  \return       void

  
 */
/*--------------------------------------------------------------------------*/
void load_qr();
/*-------------------------------------------------------------------------*/
/**
  \brief        Frees associated memory and clears QR List
  \param        list    The Quarantine list to clear
  \return       void

  
 */
/*--------------------------------------------------------------------------*/
void clear_qr_list();
/*-------------------------------------------------------------------------*/
/**
  \brief        Frees associated memory and clears QR List
  \param        list    The Quarantine list to clear
  \return       void

  
 */
/*--------------------------------------------------------------------------*/
void clear_tmp_qr_list(QrList** list);
/*-------------------------------------------------------------------------*/
/**
  \brief        Function to find a file in the Quarantine
  \param        list        The quarantine list
  \param        filename    The file to find
  \return       The searched QrListNode

  
 */
/*--------------------------------------------------------------------------*/
QrListNode* search_in_qr(char *filename);
/*-------------------------------------------------------------------------*/
/**
  \brief        Saves the Quarantine List
  \param        list        The quarantine list
  \param        custom      If set, custom save location
  \return       0 on success, -1 on error

  
 */
/*--------------------------------------------------------------------------*/
int save_qr_list(int custom);
/*-------------------------------------------------------------------------*/
/**
  \brief            Add a file to the Quarantine, physically and logically
  \param            list        The Quarantine list
  \param            filepath    The file to add
  \return           0 on success, -1 on error

  
 */
/*--------------------------------------------------------------------------*/
int add_file_to_qr(char *filepath);
/*-------------------------------------------------------------------------*/
/**
  \brief        Removes a file from quarantine physically and logically
  \param        list        The Quarantine list
  \param        filename    The file to remove
  \return       0 on success, -1 on error

  
 */
/*--------------------------------------------------------------------------*/
int rm_file_from_qr(char *filename);
/*-------------------------------------------------------------------------*/
/**
  \brief        Restore file to its anterior state and place
  \param        list        The Quarantine list
  \param        filename    The file to restore
  \return       0 on success, -1 on error

  
 */
/*--------------------------------------------------------------------------*/
int restore_file(char *filename);
/*-------------------------------------------------------------------------*/
/**
  \brief    Main function of qr-worker process
  \return   0 on success, -1 on error

  
 */
/*--------------------------------------------------------------------------*/
void qr_worker();
/*-------------------------------------------------------------------------*/
/**
  \brief    Print all elements contained in qr-list to stdout
  \param    list        The Quarantine list
  \return   void

  
 */
/*--------------------------------------------------------------------------*/
void print_qr();

/*-------------------------------------------------------------------------*/
/**
  \brief        Manages the Expired Files functionality
  \param        list  The quarantine list to iterate
  \return       void

  Function of process who is tasked with deleting files 
  earmarked by a deletion date older than todays date time.
  Loops until no more expired files are detected
 */
/*--------------------------------------------------------------------------*/
void expired_files();

void exit_quarantine();

#endif /* _KLEARNEL_QUARANTINE_H */
