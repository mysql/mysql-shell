#@ __global__
||

#@<OUT> colladd
NAME
      CollectionAdd - Operation to insert documents into a Collection.

DESCRIPTION
      A CollectionAdd object represents an operation to add documents into a
      Collection, it is created through the add function on the Collection
      class.

FUNCTIONS
      add(...)
            Adds documents into a collection.

      execute()
            Executes the add operation, the documents are added to the target
            collection.

      help([member])
            Provides help about this class and it's members

#@<OUT> colladd.add
NAME
      add - Adds documents into a collection.

SYNTAX
      <CollectionAdd>.add(documentList)
      <CollectionAdd>.add(document[, document, ...])
      <CollectionAdd>.add(mysqlx.expr(...))

RETURNS
       This CollectionAdd object.

DESCRIPTION
      This function receives one or more document definitions to be added into
      a collection.

      A document definition may be provided in two ways:

      - Using a dictionary containing the document fields.
      - Using A JSON string as a document expression.

      There are three ways to add multiple documents:

      - Passing several parameters to the function, each parameter should be a
        document definition.
      - Passing a list of document definitions.
      - Calling this function several times before calling execute().

      To be added, every document must have a string property named '_id'
      ideally with a universal unique identifier (UUID) as value. If the '_id'
      property is missing, it is automatically set with an internally generated
      UUID.

      JSON as Document Expressions

      A document can be represented as a JSON expression as follows:

      mysqlx.expr(<JSON object>)

      The JSON object parameter must be a string representing a JSON object.

EXAMPLES
      collection.add({"name":"John", "age":25})
            Inserts a document from a dictionary.

      collection.add(mysqlx.expr('{"name":"John", "age":25}'))
            Inserts a document created from a JSON String.

#@<OUT> colladd.execute
NAME
      execute - Executes the add operation, the documents are added to the
                target collection.

SYNTAX
      <CollectionAdd>.execute()

RETURNS
       A Result object.

#@<OUT> colladd.help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <CollectionAdd>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

