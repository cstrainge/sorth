
{}.new variable! ds.document_store
( ds.symbol_store )



# ds.document uri highlight_list version contents ;


: ds.document.uri!             ds.document.uri            swap @ #! ;
: ds.document.highlight_list!  ds.document.highlight_list swap @ #! ;
: ds.document.version!         ds.document.version        swap @ #! ;
: ds.document.contents!        ds.document.contents       swap @ #! ;

: ds.document.uri@             ds.document.uri            swap @ #@ ;
: ds.document.highlight_list@  ds.document.highlight_list swap @ #@ ;
: ds.document.version@         ds.document.version        swap @ #@ ;
: ds.document.contents@        ds.document.contents       swap @ #@ ;


: ds.document.new  ( uri version contents -- ds.document )
    ds.document.new variable! document

    document ds.document.contents!
    document ds.document.version!
    document ds.document.uri!

    document @
;


: ds.document.generate_highlights  ( ds.document -- )
    drop
;


: ds.document.generate_symbols  ( ds.document -- )
    drop
;




: ds.insert_document  ( uri version contents -- )
    ds.document.new variable! new_document

    new_document @ document_store { new_document ds.document.uri@ }@@
    new_document @ ds.document.generate_highlights
    new_document @ ds.document.generate_symbols
;
