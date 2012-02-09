# Nginx Delay Module

## Introduction

The `ngx_http_delay` module is authored by
[Maxim Dounin](http://mdounin.ru), member of the core
[Nginx](http://nginx.org) team.

Maxim mantains a mercurial
[repository](http://mdounin.ru/hg/ngx_http_delay_module) with
the latest version of the code.

This module enables a `delay` directive to be used when serving a
request. This can be of use for limiting the rate of requests without
signaling a `503 Service Unavailable` to the client.

It can also be used to perform abuse control. It's not hard to imagine
a script that coordinate with a TCP traffic analysis reloads the
Nginx configuration inserting a `delay` directive wherever necessary.

Here's an example configuration:

    location / {
        error_page 418 @abuse;
        if ($is_abusing) {
            return 418;
        }
        # other stuff here...
    }
    
    ## Abusers get a delay of 5s.
    location @abuse {
        delay 5s;
        
        # other stuff here...
    }
    
the `$is_abusing` variable gets a non-zero value following a criteria
you choose.

You can **compound** delays by issuing an internal redirect. In the
above example if we have a serious abuser we can do a further redirect
to get a **10s delay** right of the bat.

    recursive_error_pages on; 

    location @abuse {
        delay 5s;
        
        error_page 418 @serious-abuse;
        if ($is_serious_abuser) {
            return 418;
        }
        
        # other stuff here...
    }
    
    location @serious-abuse {
        delay 1m; # delay 1 minute
        
        # other stuff here...
    }
    
So he have a 10s + 1m delay, giving a 70s total delay.

## Module directives

**delay** `<time specification>` 

**default:** `0`

**context:** `http`, `server`, `location`

Inserts a delay of the specified duration when serving a request.

## Installation

 1. Grab the source from the mercurial
    [tip](http://mdounin.ru/hg/ngx_http_delay_module/archive/tip.tar.gz)
    or clone this repo:

        git clone git://github.com/perusio/nginx-delay-module.git
    
 2.  2. Add the module to the build configuration by adding
    `--add-module=/path/to/ngx_http_delay_module`.
    
 3. Build the nginx binary.
 
 4. Install nginx binary.
 
 5. Configure contexts where `delay` is enabled.
 
 6. Done.

## Other possible uses

One other possible use is for doing poor man's
[traffic shapping](https://en.wikipedia.org/wiki/Traffic_shaping).

## Opinionated scholium 

This is yet another example of using a very simple module that can
perform an action useful in security terms. There's no need for
bloated `mod_security` like mess.

## Other Maxim Dounin Nginx modules on Github

 + [Nginx Auth Request](https://github.com/perusio/nginx-auth-request-module):
   allows for subrequests in an authorization. Can be used for request
   filtering or two-factor authentication for example.
