// Copyright Verizon Media. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package ai.vespa.feed.client;

import org.apache.hc.client5.http.async.methods.SimpleHttpRequest;
import org.apache.hc.client5.http.async.methods.SimpleHttpResponse;

import java.util.concurrent.CompletableFuture;
import java.util.function.BiConsumer;

/**
 * Controls execution of feed operations.
 *
 * @author jonmv
 */
public interface RequestStrategy {

    /** Whether this has failed fatally, and we should cease sending further operations. */
    boolean hasFailed();

    /** Enqueue the given operation, returning its future result. This may block if the client send queue is full. */
    CompletableFuture<SimpleHttpResponse> enqueue(DocumentId documentId, SimpleHttpRequest request,
                                                  BiConsumer<SimpleHttpRequest, CompletableFuture<SimpleHttpResponse>> dispatch);

}
