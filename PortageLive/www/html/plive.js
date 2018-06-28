/**
 * @author Samuel Larkin
 * @file plive.js
 * @brief plive client-side controller.
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Centre de recherche en technologies numériques / Digital Technologies Research Centre
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2018, Sa Majeste la Reine du Chef du Canada
 * Copyright 2018, Her Majesty in Right of Canada
 */



// https://coderwall.com/p/flonoa/simple-string-format-in-javascript
/*
if (!String.prototype.format) {
   String.prototype.format = function() {
      a = this;
      for (k in arguments) {
         a = a.replace("{" + k + "}", arguments[k])
      }
      return a
   }
}
*/


// https://stackoverflow.com/a/4673436
/*
if (!String.prototype.format) {
   String.prototype.format = function() {
      const args = arguments;
      return this.replace(/{(\d+)}/g, function(match, number) {
         return typeof args[number] != 'undefined' ? args[number] : match ;
      });
   };
}
*/

if (!String.prototype.format) {
   String.prototype.format = function() {
      const args = arguments[0];  // To extract the dictionary.
      return this.replace(/{([a-zA-Z0-9_]+)}/g, function(match, number) {
         return typeof args[number] != 'undefined' ? args[number] : match ;
      });
   };
}

/*
// If you prefer not to modify String's prototype:
if (!String.format) {
   String.format = function(format) {
      var args = Array.prototype.slice.call(arguments, 1);
      return format.replace(/{(\d+)}/g, function(match, number) {
         return typeof args[number] != 'undefined' ? args[number] : match ;
      });
   };
}
*/



// https://stackoverflow.com/a/7918944
if (!String.prototype.encodeHTML) {
   String.prototype.encodeHTML = function () {
      return this.replace(/&/g, '&amp;')
         .replace(/</g, '&lt;')
         .replace(/>/g, '&gt;')
         .replace(/"/g, '&quot;')
         .replace(/'/g, '&apos;')
         .replace(/\//g, '&#x2F;');
   };
}



// https://css-tricks.com/intro-to-vue-2-components-props-slots/
// https://alligator.io/vuejs/component-communication/
Vue.component('translating',
   {
      props: ['is_translating'],
      template: '#translating_template'
});



//Vue.use(Toasted);
Vue.use(Toasted, {
   iconPack : 'material' // set your iconPack, defaults to material. material|fontawesome
});



Vue.toasted.register('error', message => message, {
   duration : 3000,
   fullWidth : true,
   icon : 'error',
   iconPack: 'material',
   position: 'bottom-center',
   theme: 'bubble',
   type: 'error',
});



Vue.toasted.register('success', message => message, {
   duration : 3000,
   fullWidth : true,
   icon : 'done_outline',
   iconPack: 'material',
   position: 'bottom-center',
   theme: 'bubble',
   type: 'success',
});



var plive_app = new Vue({
   el: '#plive_app',

   data: {
      service_url: '/PortageLiveAPI.php',
      contexts: [],
      filters: [],
      version: '',
      context: 'unselected',
      text_source: '',
      text_xtags: false,
      document_id: '',
      enable_phrase_table_debug: false,
      translation: '',
      is_translating_text: false,
      is_translating_file: false,
      incr_source_segment: '',
      incr_target_segment: '',
      newline: 'p',
      pretokenized: false,
      file: undefined,
      is_xml: false,
      translation_url: undefined,
      oov_url: undefined,
      pal_url: undefined,
      translation_progress: 0,
      trace_url: undefined,
      translate_file_error: '',
      prime_mode: 'partial',
      incrStatus_status: undefined,
      primeModel_status: undefined,
   },

   // On page loaded...
   mounted: function() {
      let app = this;
      this._createFilters();
      this.getAllContexts();
      this.getVersion();
      let myToastFailed = app.$toasted.global.error('<i class="fa fa-car"></i>My custom error message');
      let myToastSuccess = app.$toasted.global.success('Successfully translate your file! <i class="fa fa-file"></i><i class="fa fa-file-text"></i><i class="fa fa-edit"></i><i class="fa fa-keyboard-o"></i> <i class="fa fa-pencil"></i>', {duration: 10000});

      if (false) {
         /*
         <script src="https://cdn.jsdelivr.net/npm/tinysoap@1.0.2/tinysoap-browser-min.js"></script>
         <script type="text/javascript" src="tinysoap-browser.js"></script>
         */
         const wsdl = '/PortageLiveAPI.wsdl';
         tinysoap.createClient(wsdl, function(err, client) {
            // First function call test.
            client.getAllContexts(
               {
                  'verbose':false,
                  'json':true
               },
               function(err, result) {
                  var c = JSON.parse(result['contexts']);
                  console.log(c)
               });
            // Second function call test.
            client.getVersion(
               {},
               function(err, result) {
                  var c = result['contexts'];
                  console.log(c)
               });
         });
      }
   },

   methods: {
      _body: function(method, args) {
         return '<' + method + '>' + Object.keys(args).reduce(function(previous, current) {
            previous += '<' + current + '>' + String(args[current]).encodeHTML() + '</' + current + '>';
            return previous;
         }, '') + '</' + method + '>';
      },

      _createFilters: function() {
         this.filters = Array.apply(null, {length: 20})
                             .map(function(value, index) {
                                return index * 5 / 100;
                             });
      },

      _fetch: function(soapaction, args) {
         // https://stackoverflow.com/questions/37693982/how-to-fetch-xml-with-fetch-api
         // https://stackoverflow.com/questions/44401177/xml-request-and-response-with-fetch
         // Response.text() returns a Promise, chain .then() to get the Promise value of .text() call.
         //
         // If you are expecting a Promise to be returned from getPostagePrice function, return fetch() call from getPostagePrice() call.
         const app = this;
         const envelope = '<?xml version="1.0" encoding="UTF-8"?> \
                          <soap:Envelope \
                             xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" \
                             xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" \
                             xmlns:xsd="http://www.w3.org/2001/XMLSchema"> \
                             <soap:Header> \
                             </soap:Header> \
                             <soap:Body> \
                                {body} \
                             </soap:Body> \
                          </soap:Envelope>'.format({'body': app._body(soapaction, args)});

         return fetch(app.service_url, {
            method: 'POST',
            headers: {
               'SOAPAction': soapaction,
               'Content-Type': 'text/xml; charset=utf-8',
               },
            body: envelope,
            })
            .then(function(response) {
               /*
               if (!response.ok) {
                  throw Error(response.statusText);
               }
               */
               return response.text();
            })
            .then(function(response) {
               return $.xml2json(response);
            })
            .then(function(response) {
               if (!!response.Body.Fault) {
                  throw Error(response.Body.Fault.faultcode + ': ' + response.Body.Fault.faultstring);
               }
               return response;
            });
      },

      _getBase64: function(file) {
         return new Promise(function(resolve, reject) {
               const reader = new FileReader();
               reader.onload = function() { resolve(reader.result) };
               reader.onerror = function(error) { reject(error) };
               reader.readAsDataURL(file);
               })
            //data:[<mime type>][;charset=<charset>][;base64],<encoded data>
            //data:*/*;base64,
            //data:;base64,
            .then( function(data) {
               return data.replace(/^data:(.*\/.*)?;base64,/g, '');
            } );
      },

      _getContext: function() {
         const app = this;
         var context = app.context;
         if (app.document_id !== undefined && app.document_id !== '') {
            context = context + '/' + app.document_id;
         }
         return context;
      },


      clearForm: function() {
         const app = this;

         app.context = 'unselected';
         app.text_source = '';
         app.text_xtags = false;
         app.document_id = '';
         app.enable_phrase_table_debug = false;
         app.translation = '';
         app.is_translating_text = false;
         app.is_translating_file = false;
         app.incr_source_segment = '';
         app.incr_target_segment = '';
         app.newline = 'p';
         app.pretokenized = false;
         app.file = undefined;
         app.is_xml = false;
         app.translation_url = undefined;
         app.oov_url = undefined;
         app.pal_url = undefined;
         app.translation_progress = -1;
         app.trace_url = undefined;
         app.translate_file_error = '';
         app.prime_mode = 'partial';
         app.incrStatus_status = undefined;
         app.primeModel_status = undefined;
      },

      getAllContexts: function() {
         const app = this;

         return app._fetch('getAllContexts', {'verbose': false, 'json': true})
            .then(function(response) {
               // We've asked for a json replied which is embeded in the xml response.
               const jsonContexts = JSON.parse(response.Body.getAllContextsResponse.contexts);
               const contexts = jsonContexts.contexts.reduce(function(acc, cur, i) {
                     acc[cur.name] = cur;
                     return acc;
                  },
                  {});
               app.contexts = contexts;
               app.context = 'unselected';
            })
            .catch(function(err) {
               alert("Error fetching available contexts/systems from the server. " + err);
            });
      },

      getVersion: function() {
         // https://stackoverflow.com/questions/37693982/how-to-fetch-xml-with-fetch-api
         // https://stackoverflow.com/questions/44401177/xml-request-and-response-with-fetch
         // Response.text() returns a Promise, chain .then() to get the Promise value of .text() call.
         //
         // If you are expecting a Promise to be returned from getPostagePrice function, return fetch() call from getPostagePrice() call.
         const app = this;

         return app._fetch('getVersion', {})
            .then(function(response) {
               app.version = String(response.Body.getVersionResponse.version);
               console.log(app.version);
            })
            .catch(function(err) {
               app.version = "Failed to retrieve Portage's version";
               alert("Failed to get Portage's version." + err);
            });
      },

      incrAddSentence: function() {
         const app = this;

         return app._fetch('incrAddSentence', {
               context: app.context,
               document_model_id: app.document_id,
               source_sentence: app.incr_source_segment,
               target_sentence: app.incr_target_segment,
               extra_data: '',
            })
            .then(function(response) {
               const status = response.Body.incrAddSentenceResponse.status;
               app.incr_source_segment = '';
               app.incr_target_segment = '';
               // TODO: There is no feedback if the status is false.
               let myToast = app.$toasted.global.success('Successfully added sentence pair!<i class="fa fa-send"></i>');
            })
            .catch(function(err) {
               const faultstring = response.Body.Fault.faultstring;
               const faultcode = response.Body.Fault.faultcode;
               alert("Error adding sentence pair." + faultstring);
            });
      },

      incrStatus: function() {
         const app = this;

         return app._fetch('incrStatus', {
            'context': app.context,
            'document_model_id': app.document_id,
            })
            .then(function(response) {
               const status = app._getContext()
                  + String(response.Body.incrStatusResponse.status_description);
               app.incrStatus_status = status;
               let myToast = app.$toasted.global.success(status + '<i class="fa fa-cogs"></i>');
               // TODO: Show the incremental status
            })
            .catch(function(err) {
               alert("Failed to get incremental status " + app.context + '\n' + err);
            });
      },

      prepareFile: function(evt) {
         // Inspiration: https://stackoverflow.com/questions/36280818/how-to-convert-file-to-base64-in-javascript
         // https://developer.mozilla.org/en-US/docs/Web/API/FileReader/readAsDataURL
         const app   = this;
         const files = evt.target.files;
         const file  = files[0];

         // UI related.
         app.translation_progress = 0;
         app.translation_url      = undefined;
         app.trace_url            = undefined;
         app.translate_file_error = '';

         app._getBase64(file)
            .then( function(data) {
               app.file = file;
               app.file.base64 = data;
               if (/\.(sdlxliff|xliff)$/i.test(file.name)) {
                  app.is_xml = true;
                  app.file.translate_method = 'translateSDLXLIFF';
               }
               else if (/\.tmx$/i.test(file.name)) {
                  app.is_xml = true;
                  app.file.translate_method = 'translateTMX';
               }
               else if (/\.txt$/i.test(file.name)) {
                  app.is_xml = false;
                  app.file.translate_method = 'translatePlainText';
               }
               else {
                  app.is_xml = false;
                  app.file.translate_method = 'translatePlainText';
               }
            })
            .catch( function(error) {
               alert("Error converting your file to base64!");
            } );
      },

      primeModel: function() {
         const app = this;
         // Let's capture the context in case the user changes context before
         // priming is done.

         // UI related.
         app.primeModel_status = 'priming';

         return app._fetch('primeModels', {
               'context': app.context,
               'PrimeMode': app.prime_mode,
            })
            .then(function(response) {
               const status = String(response.Body.primeModelsResponse.status);
               if (status === 'true') {
                  app.primeModel_status = 'successful';
                  let myToast = app.$toasted.global.success('Successfully primed ' + app.context + '!<i class="fa fa-tachometer"></i>');
               }
               else {
                  app.primeModel_status = 'failed';
                  let myToast = app.$toasted.global.error('Failed to prime ' + app.context + '!<i class="fa fa-tachometer"></i>');
               }
            })
            .catch(function(err) {
               alert("Failed to prime context " + app.context + soapResponse.toJSON());
            });
      },

      translateFile: function(evt) {
         const app = this;
         const data = {
            ContentsBase64: app.file.base64,
            Filename: app.file.name,
            context: app._getContext(),
            useCE: false,
            xtags: app.text_xtags,
         };

         if (app.is_xml) {
            data.CETreshold = 0;
         }

         // UI related.
         app.translation_progress = 0;
         app.translation_url      = undefined;
         app.trace_url            = undefined;
         app.translate_file_error = '';

         app.is_translating_file = true;

         return app._fetch(app.file.translate_method, data)
            .then(function(response) {
               app.translateFileSuccess(response, app.file.translate_method + 'Response');
            })
            .catch(function(err) {
               app.is_translating_file = false;
               alert('Failed to translate your file!' + soapResponse.toJSON());
            });
      },

      translateFileSuccess: function (soapResponse, method) {
         const app = this;
         var response = soapResponse.Body;
         var token = response[method].token;
         // IMPORTANT we want to be able to stop the setInterval from
         // within the setInterval callback function thus we have to have
         // `watcher` in the callback's scope.
         const watcher = setInterval(
            function(monitor_token) {
               return app._fetch('translateFileStatus', { token: String(monitor_token) })
                  .finally( function() {
                     app.is_translating_file = false;
                  })
                  .then(function(response) {
                     var token = response.Body.translateFileStatusResponse.token;
                     if (token.startsWith('0 Done:')) {
                        window.clearInterval(watcher);
                        app.translation_progress = 100;
                        app.translation_url = token.replace(/^0 Done: /, '/');
                        app.pal_url = app.translation_url.replace(/[^\/]+$/, 'pal.html');
                        app.oov_url = app.translation_url.replace(/[^\/]+$/, 'oov.html');
                        let myToast = app.$toasted.global.success('Successfully translate your file ' + app.file.name + '!<i class="fa fa-file-text"></i>');
                     }
                     else if (token.startsWith('1')) {
                        // TODO: indicate progress.
                        // "1 In progress (0% done)"
                        const per = token.match(/1 In progress \((\d+)% done\)/);
                        if (per) {
                           app.translation_progress = per[1];
                        }
                        app.is_translating_file = true;  // We are actually still translating
                     }
                     else if (token.startsWith('2 Failed')) {
                        window.clearInterval(watcher);
                        // 2 Failed - no sentences to translate : plive/SOAP_BtB-METEO.v2.E2F_Devoirdephilo2.docx.xliff_20180503T152059Z_mBJdDf/trace
                        const messages = token.match(/2 Failed - ([^:]+) : (.*)/);
                        // TODO: we should be checking the length of messages.
                        if (messages) {
                           app.translate_file_error = messages[1];
                           app.trace_url = '/' + messages[2];
                        }
                     }
                     else if (token.startsWith('3')) {
                        window.clearInterval(watcher);
                     }
                     else {
                        window.clearInterval(watcher);
                     }
                  })
                  .catch(function(err) {
                     alert('Failed to retrieve your translation status!' + err);
                  });
            },
            5000,
            token);
      },

      translate: function() {
         const app = this;
         app.translation = '';
         const is_incremental = app.contexts[app.context].is_incremental;
         if (app.document_id !== undefined && app.document_id !== '' && !is_incremental) {
            alert(app.context + " does not support incremental.  Please select another system.");
            return;
         }

         app.is_translating_text = true;

         return app._fetch('translate', {
               srcString: app.text_source,
               context: app._getContext(),
               newline: app.newline,
               xtags: app.text_xtags,
               useCE: false,
            })
            .finally( function() {
               app.is_translating_text = false;
            })
            .then(function(response) {
               app.translation = response.Body.translateResponse.Result;
               let myToast = app.$toasted.global.success('Successfully translated your text!<i class="fa fa-keyboard-o"></i>');
            })
            .catch(function(err) {
               alert('Failed to translate your sentences!' + err);
               let myToast = app.$toasted.global.error('Failed to translate your text!<i class="fa fa-keyboard-o"></i>');
            });
      },

      translate_using_REST_API: function () {
         // https://developers.google.com/web/updates/2015/03/introduction-to-fetch
         // https://developer.mozilla.org/en-US/docs/Web/API/WindowOrWorkerGlobalScope/fetch
         this.translation_segments = null;
         const request = {
            'options': {
               'sent_split': new Boolean(true),
            },
            'segments': [
               this.source_segment
            ]};
         fetch('/translate', {
            method: 'POST',
            headers: {
               'Accept': 'application/json',
               'Content-Type': 'application/json',
               },
            body: JSON.stringify(request),
            })
         .then(function(response) { response.json() } )
         .then(function(translations) {
            this.translation_segments = translations;
            console.log(translations);})
         .catch(function(err) { console.error(err) } )
      },
   }  // methods
});



Vue.filter('doubleDigits', function (value) {
    if (typeof value !== "number") {
        return value;
    }
    const formatter = new Intl.NumberFormat('en-US', {
        style: "decimal",
        //maximumSignificantDigits: 2,
        minimumFractionDigits: 2
    });
    return formatter.format(value);
});
