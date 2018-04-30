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
      return this.replace(/{([a-zA-Z0-9]+)}/g, function(match, number) {
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



// https://css-tricks.com/intro-to-vue-2-components-props-slots/
// https://alligator.io/vuejs/component-communication/
Vue.component('translating',
   {
      props: ['is_translating'],
      template: '#translating_template'
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
      document_id: undefined,
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
      this.createFilters();
      this.getAllContexts();
      this.getVersion();
      const request = $.soap({timeout: 300000});  // Some of PORTAGELive calls can't take a long time.
   },

   methods: {
      getContext: function() {
         const app = this;
         var context = app.context;
         if (app.document_id !== undefined && app.document_id !== '') {
            context = context + '/' + app.document_id;
         }
         return context;
      },

      createFilters: function() {
         this.filters = Array.apply(null, {length: 20})
                             .map(function(value, index) {
                                return index * 5 / 100;
                             });
      },

      getAllContexts: function() {
         const app = this;
         const request = $.soap({
            url: app.service_url,
            method: 'getAllContexts',
            appendMethodToURL: false,

            data: {
               verbose: true,
               json: true
            },

            success: function (soapResponse) {
               const response = soapResponse.toJSON();
               // We've asked for a json replied which is embeded in the xml response.
               const jsonContexts = JSON.parse(response.Body.getAllContextsResponse.contexts);
               const contexts = jsonContexts.contexts.reduce(function(acc, cur, i) {
                     acc[cur.name] = cur;
                     return acc;
                  },
                  {});
               app.contexts = contexts;
               app.context = 'unselected';
            },

            error: function (soapResponse) {
               alert("Error fetching available contexts/systems from the server. " + soapResponse.toJSON());
            }
         });
      },

      getBase64: function(file) {
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

         app.getBase64(file)
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

      translateFile: function(evt) {
         const app = this;
         const data = {
            ContentsBase64: app.file.base64,
            Filename: app.file.name,
            context: app.getContext(),
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

         const request = $.soap({
            url: app.service_url,
            method: app.file.translate_method,
            appendMethodToURL: false,

            data: data,

            success: function (soapResponse) {
               app.translateFileSuccess(soapResponse, app.file.translate_method + 'Response');
            },

            error: function (soapResponse) {
               app.is_translating_file = false;
               alert('Failed to translate your file!' + soapResponse.toJSON());
            }
         });

         app.is_translating_file = true;
      },

      translateFileSuccess: function (soapResponse, method) {
         const app = this;
         var response = soapResponse.toJSON().Body;
         var token = response[method].token;
         // IMPORTANT we want to be able to stop the setInterval from
         // within the setInterval callback function thus we have to have
         // `watcher` in the callback's scope.
         const watcher = setInterval(
            function(monitor_token) {
               const request = $.soap({
                  url: app.service_url,
                  method: 'translateFileStatus',
                  appendMethodToURL: false,

                  data: {
                     token: String(monitor_token),
                  },

                  success: function (soapResponse) {
                     var response = soapResponse.toJSON();
                     var token = response.Body.translateFileStatusResponse.token;
                     if (token.startsWith('0 Done:')) {
                        window.clearInterval(watcher);
                        app.translation_progress = 100;
                        app.translation_url = token.replace(/^0 Done: /, '/');
                        app.pal_url = app.translation_url.replace(/[^\/]+$/, 'pal.html');
                        app.oov_url = app.translation_url.replace(/[^\/]+$/, 'oov.html');
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
                  },

                  error: function (soapResponse) {
                     alert('Failed to retrieve your translation status!' + soapResponse.toJSON());
                  }
               })
               .always( function() { app.is_translating_file = false } );
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

         const request = $.soap({
            url: app.service_url,
            method: 'translate',
            appendMethodToURL: false,

            data: {
               srcString: app.text_source,
               context: app.getContext(),
               newline: app.newline,
               xtags: app.text_xtags,
               useCE: false,
            },

            success: function (soapResponse) {
               const response = soapResponse.toJSON();
               app.translation = response.Body.translateResponse.Result;
            },

            error: function (soapResponse) {
               alert('Failed to translate your sentences!' + soapResponse.toJSON());
            }
         })
         .always( function() {
            app.is_translating_text = false;
         });

         app.is_translating_text = true;
      },

      incrAddSentence: function() {
         const app = this;
         const request = $.soap({
            url: app.service_url,
            method: 'incrAddSentence',
            appendMethodToURL: false,

            data: {
               context: app.context,
               document_model_id: app.document_id,
               source_sentence: app.incr_source_segment,
               target_sentence: app.incr_target_segment,
               extra_data: '',
            },

            success: function (soapResponse) {
               const response = soapResponse.toJSON();
               const status = response.Body.incrAddSentenceResponse.status;
               app.incr_source_segment = '';
               app.incr_target_segment = '';
               // TODO: There is no feedback if the status is false.
            },

            error: function (soapResponse) {
               const response = soapResponse.toJSON();
               const faultstring = response.Body.Fault.faultstring;
               const faultcode = response.Body.Fault.faultcode;
               alert("Error adding sentence pair." + faultstring);
            }
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

      getVersion: function() {
         const app = this;

         const request = $.soap({
            url: app.service_url,
            method: 'getVersion',
            appendMethodToURL: false,
            data: {},

            success: function(soapResponse) {
               const response = soapResponse.toJSON();
               app.version = String(response.Body.getVersionResponse.version);
            },

            error: function (soapResponse) {
               app.version = "Failed to retrieve Portage's version";
               alert("Failed to get Portage's version." + soapResponse.toJSON());
            }
         });
      },

      primeModel: function() {
         const app = this;
         // Let's capture the context in case the user changes context before
         // priming is done.
         const request_data = {
               'context': app.context,
               'PrimeMode': app.prime_mode,
            };

         // UI related.
         app.primeModel_status = 'priming';

         const request = $.soap({
            url: app.service_url,
            method: 'primeModels',
            appendMethodToURL: false,
            data: request_data,

            success: function(soapResponse) {
               const response = soapResponse.toJSON();
               const status = String(response.Body.primeModelsResponse.status);
               if (status === 'true') {
                  app.primeModel_status = 'successful';
               }
               else {
                  app.primeModel_status = 'failed';
               }
            },

            error: function (soapResponse) {
               alert("Failed to prime context " + app.context + soapResponse.toJSON());
            }
         });
      },

      incrStatus: function() {
         const app = this;
         app.incrStatus_status = undefined;

         const request = $.soap({
            url: app.service_url,
            method: 'incrStatus',
            appendMethodToURL: false,
            data: {
               "context": app.context,
               "document_model_ID": app.document_id,
            },

            success: function(soapResponse) {
               const response = soapResponse.toJSON();
               const status = String(response.Body.incrStatusResponse.status_description);
               app.incrStatus_status = status;
               // TODO: Show the incremental status
            },

            error: function (soapResponse) {
               alert("Failed to prime context " + app.context + soapResponse.toJSON());
            }
         });
      },

      clearForm: function() {
         const app = this;

         app.context = 'unselected';
         app.text_source = '';
         app.text_xtags = false;
         app.document_id = undefined;
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
