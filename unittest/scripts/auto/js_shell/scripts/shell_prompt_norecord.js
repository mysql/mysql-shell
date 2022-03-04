//@<> Setup
let confirm_options = [{yes:"&Yes"}, {no: "&No"}, {alt:"Other"}]
let prompt_text_types = [{}, {type:"text"}, {type:"fileopen"}, {type:"filesave"}, {type:"directory"}]
let prompt_types = [...prompt_text_types, {type:"password"}, {type:"confirm"}, {type:"select"}]


//@<> TEST Invalid prompt type
EXPECT_THROWS(function() {
    shell.prompt("Invalid Prompt Type", {type:"unexisting"});
}, "Shell.prompt: Argument #2: Invalid prompt type: 'unexisting'.");


//@<> TEST valid prompt types with invalid options
for(type_index in prompt_types){
    let type = prompt_types[type_index]

    // TEST invalid default value type
    if (type=={} || type["type"]!="select") {
        EXPECT_THROWS(function() {
            shell.prompt("Wrong default value type", {...type, defaultValue: 45});
        }, "Shell.prompt: Argument #2: Invalid data type on 'defaultValue': String expected, but value is Integer");
    } else {
        EXPECT_THROWS(function() {
            shell.prompt("Wrong default value type", {...type, defaultValue: "whatever", options:['One', 'Two']});
        }, "Shell.prompt: Argument #2: Invalid data type on 'defaultValue': UInteger expected, but value is String");
    }

    // TEST invalid confirm prompt options
    // they are only valid in 'confirm' prompts
    if (type=={} || type["type"]!="confirm") {
        for(index in confirm_options){
            EXPECT_THROWS(function() {
                shell.prompt("Invalid confirm option", {...type, ...confirm_options[index]});
            }, "Shell.prompt: Argument #2: The 'yes', 'no' and 'alt' options only can be used on 'confirm' prompts.");
        }
    }

    // TEST invalid select prompt options
    // they are only valid on 'select' prompts
    if (type=={} || type["type"]!="select") {
        EXPECT_THROWS(function() {
            shell.prompt("Wrong default value type", {...type, options: ['one', 'two', 'three']});
        }, "Shell.prompt: Argument #2: The 'options' list only can be used on 'select' prompts.");
    }
}


//-----------------------------
//        TEXT PROMPTS
//-----------------------------
//@<> TEST successful cases for text type prompts
// WL14872-TSFR_6_1
for(type_index in prompt_text_types){
    let type = prompt_text_types[type_index]

    //@<> Using user provided answer
    testutil.expectPrompt("Answer this question", "Whatever")
    result = shell.prompt("Answer this question", {...type})
    EXPECT_EQ("Whatever", result)

    //@<> Using default answer
    testutil.expectPrompt("Answer this question", "")
    result = shell.prompt("Answer this question", {...type, defaultValue:"Whatever"})
    EXPECT_EQ("Whatever", result)
}

//@ TEST prompt with paragraph
// WL14872-TSFR_2_1
testutil.expectPrompt("Answer this question", "Whatever")
result = shell.prompt("Answer this question", {title:'Ignored Title', description:["First Paragraph.", "Second paragraph."]})
EXPECT_EQ("Whatever", result)

//-----------------------------
//        CONFIRM PROMPTS
//-----------------------------
//@<> TEST Invalid defaultValue in confirm prompts
EXPECT_THROWS(function() {
    shell.prompt("Invalid Prompt Type", {type:"confirm", defaultValue:'other'});
}, "Shell.prompt: Argument #2: Invalid 'defaultValue', allowed values include: &No, &Yes, N, No, Y, Yes");

//@<> TEST confirm prompts with duplicated labels/values/shortcuts
// WL14872-TSFR_4_3
EXPECT_THROWS(function() {
    shell.prompt("Duplicated label", {type:"confirm", alt: '&no'});
}, "Shell.prompt: Argument #2: Labels, shortcuts and values in 'confirm' prompts must be unique: '&no' label is duplicated");

EXPECT_THROWS(function() {
    shell.prompt("Duplicated label", {type:"confirm", alt: 'n&o'});
}, "Shell.prompt: Argument #2: Labels, shortcuts and values in 'confirm' prompts must be unique: 'no' value is duplicated");

// WL14872-TSFR_4_4
EXPECT_THROWS(function() {
    shell.prompt("Duplicated shortcut", {type:"confirm", alt: 'A&nother'});
}, "Shell.prompt: Argument #2: Labels, shortcuts and values in 'confirm' prompts must be unique: 'n' shortcut is duplicated");

//@<> TEST invalid answers in confirm prompt using default labels
// WL14872-TSFR_4_1
testutil.expectPrompt("Are you sure about it? [y/n]: ", "whatever")
testutil.expectPrompt("Are you sure about it? [y/n]: ", "y")
result = shell.prompt("Are you sure about it?", {type: "confirm"})
EXPECT_STDOUT_CONTAINS("Please pick an option out of [y/n]:")
EXPECT_EQ("&Yes", result)

//@<> TEST invalid answers in confirm prompt using alternative label
testutil.expectPrompt("Are you sure about it? [Y]es/[N]o/[C]ancel (default No):", "whatever")
testutil.expectPrompt("Are you sure about it? [Y]es/[N]o/[C]ancel (default No):", "")
result = shell.prompt("Are you sure about it?", {type: "confirm", alt:"&Cancel", defaultValue:"&No"})
EXPECT_STDOUT_CONTAINS("Please pick an option out of [Y]es/[N]o/[C]ancel (default No):")
EXPECT_EQ("&No", result)

//@<> TEST successful cases for confirm type prompts, using default labels
let no_answers = ['', 'n', 'N', 'no', 'NO', 'No', 'nO']
for (index in no_answers) {
    testutil.expectPrompt("Are you sure about it? [y/N]: ", no_answers[index])
    result = shell.prompt("Are you sure about it?", {type: "confirm", defaultValue:"No"})
    EXPECT_EQ("&No", result)
}

let yes_answers = ['y', 'Y', 'yes', 'Yes', 'YEs', 'YES', 'yES', 'yeS', 'yEs', 'YEs', 'YeS']
for (index in yes_answers) {
    testutil.expectPrompt("Are you sure about it? [y/N]: ", yes_answers[index])
    result = shell.prompt("Are you sure about it?", {type: "confirm", defaultValue:'N'})
    EXPECT_EQ("&Yes", result)
}

//@<> TEST successful cases for confirm type prompts, using custom with default values
// WL14872-TSFR_4_2
return_vs_default_values = {'&Clone': ['&Clone', 'Clone', 'C','&clone', 'CLONE', 'c'],
                            '&Incremental': ['&Incremental', 'Incremental', 'I', '&INCREMENTAL', 'incremental', 'i'],
                            '&Skip': ['&SkIp', 'Skip', 's', '&SKip', 'skip', 'S']}

for (ret_val in return_vs_default_values) {
    for(index in return_vs_default_values[ret_val]) {
        testutil.expectPrompt("Recovery method: ", "")
        result = shell.prompt("Recovery method:", {type: "confirm", yes:"&Clone", no: "&Incremental", alt:"&Skip", defaultValue:return_vs_default_values[ret_val][index]})
        EXPECT_EQ(ret_val, result)
    }
}

//@ TEST confirm prompt with paragraph
// WL14872-TSFR_2_1
testutil.expectPrompt("Recovery method: ", "")
result = shell.prompt("Recovery method:", {type: "confirm", yes:"&Clone", no: "&Incremental", alt:"&Skip", defaultValue:'c', description:['Some explanation about the Clone recovery method.', 'Some explanation about the incremental recovery method']})
EXPECT_EQ('&Clone', result)


//-----------------------------
//        SELECT PROMPTS
//-----------------------------
//@<> TEST default value out of range
EXPECT_THROWS(function() {
    shell.prompt("Select an option", {type:"select", options:['One', 'Two', 'Three'], defaultValue:4});
}, "Shell.prompt: Argument #2: The 'defaultValue' should be the 1 based index of the default option.");


//@<> TEST option of wrong type
EXPECT_THROWS(function() {
    shell.prompt("Select an option", {type:"select", options:['One', 2, 'Three'], defaultValue:4});
}, "Shell.prompt: Argument #2: Option 'options' String expected, but value is Integer");

//@<> TEST empty options
// WL14872-TSFR_5_4
EXPECT_THROWS(function() {
    shell.prompt("Select an option", {type:"select"});
}, "Shell.prompt: Argument #2: The 'options' list can not be empty.");

EXPECT_THROWS(function() {
    shell.prompt("Select an option", {type:"select", options:[]});
}, "Shell.prompt: Argument #2: The 'options' list can not be empty.");

// WL14872-TSFR_5_3
EXPECT_THROWS(function() {
    shell.prompt("Select an option", {type:"select", options:[""]});
}, "Shell.prompt: Argument #2: The 'options' list can not contain empty or blank elements.");

EXPECT_THROWS(function() {
    shell.prompt("Select an option", {type:"select", options:["   "]});
}, "Shell.prompt: Argument #2: The 'options' list can not contain empty or blank elements.");

//@<> TEST return on user selection
// WL14872-TSFR_5_1
let options = ['One', 'Two', 'Three'];
for(index=1;index<=3;index++){
    testutil.expectPrompt("Select an option: ", index.toString())
    result = shell.prompt("Select an option: ", {type:"select", options:options});
    EXPECT_EQ(options[index-1], result)
}

//@<> TEST return on default value
// WL14872-TSFR_5_2
for(index=1;index<=3;index++){
    testutil.expectPrompt("Select an option: ", "")
    result = shell.prompt("Select an option: ", {type:"select", options:options, defaultValue:index});
    EXPECT_EQ(options[index-1], result)
}

//@ TEST select prompt with description
// WL14872-TSFR_2_1
testutil.expectPrompt("Select recovery method", "")
result = shell.prompt("Select recovery method", {type:"select", options:['Clone', 'Incremental', 'Abort'], defaultValue:1, description:['Some explanation about the Clone recovery method.', 'Some explanation about the Incremental recovery method.']});
EXPECT_EQ('Clone', result)
