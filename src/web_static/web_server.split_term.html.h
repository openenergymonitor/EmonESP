static const char CONTENT_SPLIT_TERM_HTML[] PROGMEM = 
  "<html> <head> <title>EmonESP Terminal</title> <meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"> <style> * {\n"
  "      box-sizing: border-box;\n"
  "    }\n"
  "\n"
  "    body {\n"
  "      margin: 0px;\n"
  "    }\n"
  "\n"
  "    /* Create two equal columns that floats next to each other */\n"
  "    .column {\n"
  "      float: left;\n"
  "      width: 50%;\n"
  "      height: 100%;\n"
  "    }\n"
  "\n"
  "    /* Clear floats after the columns */\n"
  "    .row:after {\n"
  "      content: \"\";\n"
  "      display: table;\n"
  "      clear: both;\n"
  "    }\n"
  "\n"
  "    /* Responsive layout - makes the two columns stack on top of each other instead of next to each other */\n"
  "    @media screen and (max-width: 1000px) {\n"
  "      .column {\n"
  "        width: 100%;\n"
  "        height: 50%;\n"
  "      }\n"
  "    } </style> </head> <body> <div class=\"row\"> <iframe class=\"column\" src=\"term.html?debug\"></iframe> <iframe class=\"column\" src=\"term.html?emontx\"></iframe> </div> </body> </html> \n";