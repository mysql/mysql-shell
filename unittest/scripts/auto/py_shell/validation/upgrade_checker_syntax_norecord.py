#@<OUT> Run upgrade checker to ensure the syntax check no longer fails with an orphaned routine
1) Check for orphaned routines and events in 5.7 (orphanedObjects)
   Error: The following objects have been orphaned. Schemas that they are
   referencing no longer exists.
   They have to be cleaned up or the upgrade will fail.

   upgrade_issues_ex.orphaned_procedure - this routine is associated to a
      schema that no longer exists.



2) MySQL syntax check for routine-like objects (syntax)
   No issues found
Errors:   1
Warnings: 0
Notices:  0

ERROR: 1 errors were found. Please correct these issues before upgrading to avoid compatibility issues.


