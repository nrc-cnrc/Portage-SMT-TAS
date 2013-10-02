How to view the PortageLiveAPI documentation:

Copy both PortageLiveAPI.wsdl.xml and wsdl-viewer.xsl in the same folder and
open the .xml file in a tool that includes an XSLT engine.  IE, Chrome and
Firefox all work, but the former two work better.  The latter is missing
the feature required to embed HTML code such as <br> and <p> in the
documentation, which we use to make the documentation more readable.

The WSDL file can also be viewed directly in SharePoint site by clicking on
PortageLiveAPI.wsdl (the .xml is hidden), since the dependent viewer file is
accessible in the same path.

Note that the .xml file is automatically generated from our source
PortageLiveAPI.wsdl file, which formally defines the API, and documents it.
Do not modify the .xml file directly!
