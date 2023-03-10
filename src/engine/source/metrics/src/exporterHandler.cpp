#include "exporterHandler.hpp"
#include "opentelemetry/exporters/zipkin/zipkin_exporter_factory.h"
#include "opentelemetry/exporters/ostream/span_exporter_factory.h"
#include "opentelemetry/exporters/memory/in_memory_span_exporter_factory.h"
#include "opentelemetry/exporters/ostream/metric_exporter.h"
#include <fstream>
#include "dataHubExporter.hpp"
#include "metrics.hpp"

std::shared_ptr<MetricsContext> ExporterHandler::handleRequest(std::shared_ptr<MetricsContext> data)
{
    create(data);
    return AbstractHandler<std::shared_ptr<MetricsContext>>::handleRequest(data);
}

void ExporterHandler::create(std::shared_ptr<MetricsContext> data)
{
    switch (data->providerType)
    {
        case ProviderTypes::Tracer:
        {
            switch (data->exporterType)
            {
                case ExportersTypes::Logging:
                {
                    if (!data->outputFile.empty())
                    {
                        data->file.open(data->outputFile);
                        data->exporter = opentelemetry::exporter::trace::OStreamSpanExporterFactory::Create(data->file);
                        break;
                    }
                    else
                    {
                        data->exporter = opentelemetry::exporter::trace::OStreamSpanExporterFactory::Create();
                        break;
                    }
                }
                case ExportersTypes::Memory:
                {
                    data->inMemorySpanData = std::make_shared<opentelemetry::exporter::memory::InMemorySpanData>(data->bufferSizeMemoryExporter);
                    data->exporter = opentelemetry::exporter::memory::InMemorySpanExporterFactory::Create(data->inMemorySpanData, data->bufferSizeMemoryExporter);
                    break;
                }
                case ExportersTypes::Zipkin:
                {
                    opentelemetry::exporter::zipkin::ZipkinExporterOptions opts;
                    data->exporter = opentelemetry::exporter::zipkin::ZipkinExporterFactory::Create(opts);
                    break;
                }
                default:
                    throw std::runtime_error {"Failure to evaluate type of exporter"};
            }
            break;
        }
        case ProviderTypes::Meter:
        {
            if (data->dataHubEnable)
            {
                auto dataHub = Metrics::instance().getDataHub();
                data->metricExporter = std::unique_ptr<opentelemetry::sdk::metrics::PushMetricExporter>(new opentelemetry::exporter::metrics::DataHubExporter(dataHub));
            }
            else if (!data->outputFile.empty())
            {
                data->file.open(data->outputFile);
                data->metricExporter = std::unique_ptr<opentelemetry::sdk::metrics::PushMetricExporter>(new opentelemetry::exporter::metrics::OStreamMetricExporter(data->file));
            }
            else
            {
                data->metricExporter = std::unique_ptr<opentelemetry::sdk::metrics::PushMetricExporter>(new opentelemetry::exporter::metrics::OStreamMetricExporter);
            }

            break;
        }
        default:
            throw std::runtime_error {"Fails to evaluate provider type"};
    }
}
