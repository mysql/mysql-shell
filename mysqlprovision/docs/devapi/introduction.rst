************
Introduction
************

The |orchestrator| is intended to help users (e.g., database administrators) 
manage database servers and perform administration tasks. For this purposes,
it supplies a set of common procedures, executed through |gadgets|, that can
be used to carry on those tasks. However, each system has its own
characteristics and particularities, requiring the execution of some
customized action that might not be covered by existing |gadgets|.
For this reason, user can use their own custom gadgets with the orchestrator.

Custom gadgets can be implemented in any programing languages, as long as an
executable program is produced to be invoked by the |orchestrator|. Therefore,
users can choose the programing language that best fits their needs to
implement a new gadget. However, being able to reuse 

In many situation, a small modification to an existing gadgets could be
enough to provide the needed adaptation and behavioral change to execute the
desired action. In this case, the |gadgets| library can be reused to make
this process easier and less time consuming. 

This part of the development documentation provides references and
descriptions of the libraries used to support the implementation of the
|orchestrator| and the |gadgets|. It is expected to be used as a reference
guide for those that intend to reuse the existing code to extend the
orchestrator features or implement new custom gadgets by reusing the
existing library.