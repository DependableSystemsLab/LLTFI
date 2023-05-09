from transformers import AutoTokenizer
import os
from onnx import numpy_helper

inputs = ["if (x is not None) <mask> (x>1)",
"public String getSecretKey ( String subdomain ) { String secretKey = secretKeys . get ( subdomain ) ; if (secretKey == <mask>) { secretKey = defaultSecretKey ; } return secretKey ; }",
"protected Iterator < Map . Entry < K , V > > createEntrySetIterator ( ) { if ( size ( ) <mask> 0 ) { return EmptyIterator . INSTANCE ; } return new EntrySetIterator < K , V > ( this ) ; }",
"public static boolean isPermanent ( ResourceModel resourceModel ) { Object resource = resourceModel . getResource ( ) ; try { return ( Boolean ) <mask> ; } catch ( ClassCastException e ) { return false ; } }",
"public void clear ( ) { modCount ++ ; HashEntry [ ] data = this . data ; for ( int i = data . length - 1 ; i <mask> 0 ; i -- ) { data [ i ] = null ; } size = 0 ; }",
"public void addPrincipal ( String principal ) { if ( ! readOnly <mask> ! principals . contains ( principal ) ) { principals . add ( principal ) ; principalsModified = true ; } }",
"protected final void addAllApplications ( Set < Class < ? > > set ) { for ( Class < ? > cls : set ) { if ( ! cls . isInterface ( ) && ! Modifier . isAbstract ( cls . getModifiers ( ) ) ) { if ( ! this . classmap . <mask> ( cls ) ) { this . classNames . add ( cls . getName ( ) ) ; } } } }",
"public void setName ( String name ) { if ( name != null && name . <mask> ( this . name ) ) { return ; } this . name = name ; Roster packet = new Roster ( ) ; packet . setType ( IQ . Type . set ) ; packet . addItem ( new JID ( user ) , name , ask , subscription , getGroupNames ( ) ) ; connection . sendPacket ( packet ) ; }",
"""public String getString ( String defaultValue ) { if ( value instanceof String || value instanceof Number ) { return value . toString ( ) ; } if ( value == null ) { return null ; } if ( value instanceof JSONArray ) { return ( ( JSONArray ) value ) . toJSONString ( ) ; } if ( value instanceof JSONObject ) { return ( ( JSONObject ) value ) . <mask> ( ) ; } if ( value == null ) { return defaultValue ; } throw createException ( "Expected string:" ) ; }""",
""" public Double getDouble ( Double defaultValue ) { if ( value instanceof Number ) { return ( ( Number ) value ) . doubleValue ( ) ; } if ( value instanceof String ) { String s = ( String ) value ; return Double . <mask> ( s ) ; } if ( value == null ) { return defaultValue ; } throw createException ( "Expected number:" ) ; }"""]

model_name = "microsoft/codebert-base-mlm"
tokenizer = AutoTokenizer.from_pretrained(model_name)

for i in range(0, len(inputs)):
    input_txt = inputs[i]
    sequence = (input_txt)
    inputs_np = tokenizer(sequence, return_tensors="np")
    tensor_input_ids = numpy_helper.from_array(inputs_np['input_ids'])
    tensor_attention_mask = numpy_helper.from_array(inputs_np['attention_mask'])

    with open(os.path.join("", f"input{i}_0.pb"), "wb") as f:
        f.write(tensor_input_ids.SerializeToString())

    with open(os.path.join("", f"input{i}_1.pb"), "wb") as f:
        f.write(tensor_attention_mask.SerializeToString())
