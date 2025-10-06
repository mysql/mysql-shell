#@<OUT> BUG38194922 false positive
1) Checks for foreign keys not referencing a full unique index
(foreignKeyReferences)
   No issues found
Errors:   0
Warnings: 0
Notices:  0

No known compatibility errors or issues were found.

#@<OUT> BUG38194922 positive check
1) Checks for foreign keys not referencing a full unique index
(foreignKeyReferences)
   Foreign keys to partial indexes may be forbidden as of 8.4.0, this check
   identifies such cases to warn the user.

   foreign_test_db.test_table_c.test_constraint_two - invalid foreign key
      defined as 'test_table_c(other_column)' references a non unique key at table
      'test_table_a'.

   Solutions:
   - Convert non unique key to unique key if values do not have any duplicates.
      In case of foreign keys involving partial columns of key, create composite 
      unique key containing all the referencing columns if values do not have any 
      duplicates.

   - Remove foreign keys referring to non unique key/partial columns of key.

   - In case of multi level references which involves more than two tables
      change foreign key reference.


Errors:   0
Warnings: 1
Notices:  0

