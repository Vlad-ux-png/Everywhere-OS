/*++

Copyright (c) 2026 O&V Soft. All Rights Reserved.

Module Name:

    workitem.c

Abstract:

    This module implements executive work items, allowing deferred 
    asynchronous execution of tasks within the kernel safely at PASSIVE_LEVEL.

Author(s):

    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel mode.

--*/

#include "ex.h"
#include "ke.h"

//
// Global work queue head for deferred system tasks.
//
static LIST_ENTRY ExpWorkQueueHead;

/*++

Routine Description:

    Initializes the executive work item subsystem queue.

Arguments:

    None.

Return Value:

    None.

--*/
VOID
ExpInitializeWorkItems(
    VOID
    )
{
    InitializeListHead(&ExpWorkQueueHead);
}

/*++

Routine Description:

    This function inserts a work item into the work queue to be 
    processed asynchronously by the system.

Arguments:

    WorkItem - Supplies a pointer to the work item to add to the queue.
        The work item structure contains a doubly linked list entry, 
        the address of a routine to call, and a parameter to pass to it.

    QueueType - Specifies the type of work queue that the work item
        should be placed in.

Return Value:

    None.

--*/
VOID
ExQueueWorkItem(
    IN PWORK_QUEUE_ITEM WorkItem,
    IN WORK_QUEUE_TYPE QueueType
    )
{
    ASSERT(QueueType < MaximumWorkQueue);
    ASSERT(WorkItem->List.Flink == NULL);

    //
    // Insert the work item at the tail of the work queue list.
    //
    InsertTailList(&ExpWorkQueueHead, &WorkItem->List);
}

/*++

Routine Description:

    Processes pending work items from the queue. Designed to be called
    safely on every iteration of the main kernel loop at PASSIVE_LEVEL.

Arguments:

    None.

Return Value:

    None.

--*/
VOID
ExpProcessWorkItems(
    VOID
    )
{
    PLIST_ENTRY Entry;
    PWORK_QUEUE_ITEM WorkItem;
    VOID (*WorkerRoutine)(PVOID);
    PVOID Parameter;

    //
    // Check if the work queue is empty before proceeding.
    //
    if (IsListEmpty(&ExpWorkQueueHead)) {
        return;
    }

    //
    // Remove the first item from the queue safely.
    //
    Entry = ExpWorkQueueHead.Flink;
    RemoveEntryList(Entry);

    WorkItem = CONTAINING_RECORD(Entry, WORK_QUEUE_ITEM, List);
    
    //
    // Clear links to indicate the work item is no longer queued.
    //
    WorkItem->List.Flink = NULL;
    WorkItem->List.Blink = NULL;

    WorkerRoutine = (VOID (*)(PVOID))WorkItem->WorkerRoutine;
    Parameter = WorkItem->Parameter;

    //
    // Execute the specified worker routine.
    //
    if (WorkerRoutine != NULL) {
        WorkerRoutine(Parameter);
    }
}
