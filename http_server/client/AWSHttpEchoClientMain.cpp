/* Copyright (c) 2009 Yahoo! Inc.  All rights reserved.
 * The copyrights embodied in the content of this file are licensed by Yahoo! Inc.
 * under the BSD (revised) open source license.
 */

#ifndef ESF_CONSOLE_LOGGER_H
#include <ESFConsoleLogger.h>
#endif

#ifndef ESF_DISCARD_ALLOCATOR_H
#include <ESFDiscardAllocator.h>
#endif

#ifndef ESF_SYSTEM_ALLOCATOR_H
#include <ESFSystemAllocator.h>
#endif

#ifndef ESF_ERROR_H
#include <ESFError.h>
#endif

#ifndef AWS_HTTP_STACK_H
#include <AWSHttpStack.h>
#endif

#ifndef AWS_HTTP_ECHO_CLIENT_CONTEXT_H
#include <AWSHttpEchoClientContext.h>
#endif

#ifndef AWS_HTTP_ECHO_CLIENT_HANDLER_H
#include <AWSHttpEchoClientHandler.h>
#endif

#ifndef AWS_HTTP_DEFAULT_RESOLVER_H
#include <AWSHttpDefaultResolver.h>
#endif

#ifndef AWS_HTTP_ECHO_CLIENT_REQUEST_BUILDER_H
#include <AWSHttpEchoClientRequestBuilder.h>
#endif

#ifndef AWS_HTTP_CLIENT_HISTORICAL_COUNTERS_H
#include <AWSHttpClientHistoricalCounters.h>
#endif

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

static void AWSRawEchoClientSignalHandler(int signal);
static volatile ESFWord IsRunning = 1;

static void printHelp(const char *progName) {
    fprintf(stderr, "Usage: %s <options>\n", progName);
    fprintf(stderr, "\n");
    fprintf(stderr, "\tOptions:\n");
    fprintf(stderr, "\t-l <logLevel>     Defaults to 7 / INFO.  See below for other levels.\n");
    fprintf(stderr, "\t-t <epollThreads> Defaults to 1 thread handing all connections.\n");
    fprintf(stderr, "\t-H <serverHost>   Defaults to localhost\n");
    fprintf(stderr, "\t-p <serverPort>   Defaults to 80\n");
    fprintf(stderr, "\t-c <connections>  Defaults to 1 connection\n");
    fprintf(stderr, "\t-i <iterations>   Defaults to 1 request per connection\n");
    fprintf(stderr, "\t-m <method>       Defaults to GET\n");
    fprintf(stderr, "\t-C <content-type> Defaults to octet-stream if there is a non-empty body\n");
    fprintf(stderr, "\t                  Otherwise no Content-Type header will be sent\n");
    fprintf(stderr, "\t-b <body file>    Defaults to an empty body\n");
    fprintf(stderr, "\t-a <abs_path>     Defaults to '/'\n");
    fprintf(stderr, "\t-o <output file>  Defaults to stdout.  Perf results will be printed here\n");
    fprintf(stderr, "\t-r                Enables persistent connections, persistent connections\n");
    fprintf(stderr, "\t                  will not be used if not specified\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "\tLog Levels:\n"
        "\tNone = 0,         All logging disabled.\n"
        "\tEmergency = 1,    System-wide non-recoverable error.\n"
        "\tAlert = 2,        System-wide non-recoverable error imminent.\n"
        "\tCritical = 3,     System-wide potentially recoverable error.\n"
        "\tError = 4,        Localized non-recoverable error.\n"
        "\tWarning = 5,      Localized potentially recoverable error.\n"
        "\tNotice = 6,       Important non-error event.\n"
        "\tInfo = 7,         Non-error event.\n"
        "\tDebug = 8         Debugging event.\n");
}

int main(int argc, char **argv) {
    int threads = 1;
    const char *host = "localhost.localdomain";
    int port = 80;
    int connections = 1;
    int iterations = 1;
    bool reuseConnections = false;
    int logLevel = ESFLogger::Info;
    const char *method = "GET";
    const char *contentType = "octet-stream";
    const char *bodyFilePath = 0;
    const char *absPath = "/";
    FILE *outputFile = stdout;

    {
        int result = 0;

        while (true) {
            result = getopt(argc, argv, "l:t:H:p:c:i:m:C:b:a:o:rh");

            if (0 > result) {
                break;
            }

            switch (result) {
            case 'l':

                logLevel = atoi(optarg);
                break;

            case 't':

                threads = atoi(optarg);
                break;

            case 'H':

                host = optarg;
                break;

            case 'p':

                port = atoi(optarg);
                break;

            case 'c':

                connections = atoi(optarg);
                break;

            case 'i':

                iterations = atoi(optarg);
                break;

            case 'm':

                method = optarg;
                break;

            case 'C':

                contentType = optarg;
                break;

            case 'b':

                bodyFilePath = optarg;
                break;

            case 'a':

                absPath = optarg;
                break;

            case 'o':

                outputFile = fopen(optarg, "w");

                if (0 == outputFile) {
                    fprintf(stderr, "Cannot open %s: %s\n", optarg, strerror(errno));
                    return -10;
                }

                break;

            case 'r':

                reuseConnections = true;
                break;

            case 'h':

                printHelp(argv[0]);
                return 0;

            default:

                printHelp(argv[0]);

                return 2;
            }
        }
    }

    ESFConsoleLogger::Initialize((ESFLogger::Severity) logLevel);
    ESFLogger *logger = ESFConsoleLogger::Instance();

    if (logger->isLoggable(ESFLogger::Notice)) {
        logger->log(
                ESFLogger::Notice,
                __FILE__,
                __LINE__,
                "Starting. logLevel: %d, threads: %d, host: %s, port: %d, connections: %d, iterations: %d, method: %s, contentType: %s, bodyFile %s, reuseConnections: %s",
                logLevel, threads, host, port, connections, iterations, method, contentType, bodyFilePath, reuseConnections ? "true" : "false");
    }

    //
    // Install signal handlers: Ctrl-C and kill will start clean shutdown sequence
    //

    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, AWSRawEchoClientSignalHandler);
    signal(SIGQUIT, AWSRawEchoClientSignalHandler);
    signal(SIGTERM, AWSRawEchoClientSignalHandler);

    //
    // Slurp the request body into memory
    //

    int bodySize = 0;
    unsigned char *body = 0;

    if (0 != bodyFilePath) {
        int fd = open(bodyFilePath, O_RDONLY);
        if (-1 == fd) {
            fprintf(stderr, "Cannot open %s: %s\n", bodyFilePath, strerror(errno));
            return -5;
        }

        struct stat statbuf;

        memset(&statbuf, 0, sizeof(statbuf));

        if (0 != fstat(fd, &statbuf)) {
            close(fd);
            fprintf(stderr, "Cannot stat %s: %s\n", bodyFilePath, strerror(errno));
            return -6;
        }

        bodySize = statbuf.st_size;

        if (0 >= bodySize) {
            close(fd);
            bodySize = 0;
            bodyFilePath = 0;
        } else {
            body = (unsigned char *) malloc(bodySize);

            if (0 == body) {
                close(fd);
                fprintf(stderr, "Cannot allocate %d bytes of memory for body file\n", bodySize);
                return -7;
            }

            int bytesRead = 0;
            int totalBytesRead = 0;

            while (totalBytesRead < bodySize) {
                bytesRead = read(fd, body + totalBytesRead, bodySize - totalBytesRead);

                if (0 == bytesRead) {
                    free(body);
                    close(fd);
                    fprintf(stderr, "Premature EOF slurping body into memory\n");
                    return -8;
                }

                if (0 > bytesRead) {
                    free(body);
                    close(fd);
                    fprintf(stderr, "Error slurping %s into memory: %s\n", bodyFilePath, strerror(errno));
                    return -9;
                }

                totalBytesRead += bytesRead;
            }

            close(fd);
        }
    }

    //
    // Create, initialize, and start the stack
    //

    AWSHttpDefaultResolver resolver(logger);
    AWSHttpClientHistoricalCounters counters(30, ESFSystemAllocator::GetInstance(), logger);

    AWSHttpStack stack(&resolver, threads, &counters, logger);

    AWSHttpEchoClientHandler handler(absPath, method, contentType, body, bodySize, connections * iterations, &stack, logger);

    // TODO - make configuration stack-specific and increase options richness
    AWSHttpClientSocket::SetReuseConnections(reuseConnections);

    ESFError error = stack.initialize();

    if (ESF_SUCCESS != error) {
        if (body) {
            free(body);
            body = 0;
        }

        return -1;
    }

    error = stack.start();

    if (ESF_SUCCESS != error) {
        if (body) {
            free(body);
            body = 0;
        }

        return -2;
    }

    ESFDiscardAllocator echoClientContextAllocator(1024, ESFSystemAllocator::GetInstance());

    sleep(1); // give the worker threads a chance to start - cleans up perf testing numbers a bit

    // Create <connections> distinct client connections which each submit <iterations> SOAP requests

    AWSHttpEchoClientContext *context = 0;
    AWSHttpClientTransaction *transaction = 0;

    for (int i = 0; i < connections; ++i) {
        // Create the request context and transaction

        context = new (&echoClientContextAllocator) AWSHttpEchoClientContext(iterations - 1);

        if (0 == context) {
            if (logger->isLoggable(ESFLogger::Critical)) {
                logger->log(ESFLogger::Critical, __FILE__, __LINE__, "[main] cannot create new client context");
            }

            if (body) {
                free(body);
                body = 0;
            }

            return -3;
        }

        transaction = stack.createClientTransaction(&handler);

        if (0 == transaction) {
            context->~AWSHttpEchoClientContext();
            echoClientContextAllocator.deallocate(context);

            if (logger->isLoggable(ESFLogger::Critical)) {
                logger->log(ESFLogger::Critical, __FILE__, __LINE__, "[main] cannot create new client transaction");
            }

            if (body) {
                free(body);
                body = 0;
            }

            return -3;
        }

        transaction->setApplicationContext(context);

        // Build the request

        error = AWSHttpEchoClientRequestBuilder(host, port, absPath, method, contentType, transaction);

        if (ESF_SUCCESS != error) {
            context->~AWSHttpEchoClientContext();
            echoClientContextAllocator.deallocate(context);
            stack.destroyClientTransaction(transaction);

            if (logger->isLoggable(ESFLogger::Critical)) {
                char buffer[100];

                ESFDescribeError(error, buffer, sizeof(buffer));

                logger->log(ESFLogger::Critical, __FILE__, __LINE__, "[main] cannot build request: %s");
            }

            if (body) {
                free(body);
                body = 0;
            }

            return -4;
        }

        // Send the request (asynch) - the context will resubmit the request for <iteration> - 1 iterations.

        error = stack.executeClientTransaction(transaction);

        if (ESF_SUCCESS != error) {
            context->~AWSHttpEchoClientContext();
            echoClientContextAllocator.deallocate(context);
            stack.destroyClientTransaction(transaction);

            if (logger->isLoggable(ESFLogger::Critical)) {
                char buffer[100];

                ESFDescribeError(error, buffer, sizeof(buffer));

                logger->log(ESFLogger::Critical, __FILE__, __LINE__, "[main] Cannot execute client transaction: %s", buffer);
            }

            if (body) {
                free(body);
                body = 0;
            }

            return error;
        }
    }

    while (IsRunning && false == handler.isFinished()) {
        sleep(5);
    }

    error = stack.stop();

    if (body) {
        free(body);
        body = 0;
    }

    if (ESF_SUCCESS != error) {
        return -3;
    }

    stack.getClientCounters()->printSummary(outputFile);

    stack.destroy();

    echoClientContextAllocator.destroy(); // echo client context destructors will not be called.

    ESFConsoleLogger::Destroy();

    fflush(outputFile);

    fclose(outputFile);

    return ESF_SUCCESS;
}

void AWSRawEchoClientSignalHandler(int signal) {
    IsRunning = 0;
}

